/*
 * uarts.c
 *
 *  Created on: 2 июн. 2026 г.
 *      Author: eugen
 */

#include "tl_common.h"

#include "BSP_uarts.h"
#include "..\def.h"
#include "..\messages.h"
#include "..\Drv\softtmrs.h"
#include "..\utils\ring_buffer.h"
#include "..\Drv\DMA.h"

typedef enum
{
    UF_TX_BUSY = BIT(0),
    UF_TX_COMPLETE_PENDING = BIT(1),
    UF_RX_ERROR = BIT(2)
} UART_Flags;

#define UART_DMA_CHANNEL_TX DMA4
#define UART_DMA_CHANNEL_RX DMA5
#define UART_RX_DMA_DATA_OFFSET 4
#define UART_RX_DMA_DATA_SIZE   (UART_RX_DMA_BUFFER_SIZE - UART_RX_DMA_DATA_OFFSET)

__attribute__((aligned(4))) static uint8_t rx_dma_buffer[2][UART_RX_DMA_BUFFER_SIZE];
__attribute__((aligned(4))) static dma_chain_config_t rx_dma_list[2];
static uint8_t rx_ring_buffer[UART_RX_RING_BUFFER_SIZE];

typedef struct
{
    uart_num_e uart;
    gpio_pin_e pin_tx;
    gpio_pin_e pin_rx;
    dma_chn_e dma_tx;
    dma_chn_e dma_rx;
    unsigned int baudrate;
    unsigned int plic;
    UART_cb cb;
    Handle ring;
    __attribute__((aligned(4))) uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
    volatile UART_Flags flags;
    volatile uint8_t rx_pending_mask;
    volatile uint16_t rx_pending_size[2];
    volatile uint8_t rx_dma_index;
} UART_Desc_t;

typedef UART_Desc_t *UART_Desc;

static UART_Desc_t uart_desc[] =
{
    {
        .uart = UART0,
        .pin_rx = GPIO_PA3,
        .pin_tx = GPIO_PA4,
        .dma_tx = UART_DMA_CHANNEL_TX,
        .dma_rx = UART_DMA_CHANNEL_RX,
        .baudrate = UART_DEFAULT_BAUDRATE,
        .plic = IRQ_UART0,
        .cb = NULL,
        .ring = NULL,
        .flags = 0,
        .rx_pending_mask = 0,
        .rx_pending_size = {0, 0},
        .rx_dma_index = 0
    }
};

static UART_Desc _get_uart_desc(uart_num_e uart)
{
    for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
        if (uart_desc[i].uart == uart)
            return &uart_desc[i];
    return NULL;
}

_attribute_ram_code_sec_noinline_ static void uart_dma_rx_callback(dma_chn_e channel)
{
    for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
    {
        UART_Desc desc = &uart_desc[i];

        if (desc->dma_rx == channel)
        {
            uint8_t completed = desc->rx_dma_index;
            uint32_t length = *(uint32_t *)rx_dma_buffer[completed];

            if (length > UART_RX_DMA_DATA_SIZE)
                length = UART_RX_DMA_DATA_SIZE;

            desc->rx_pending_size[completed] = (uint16_t)length;
            desc->rx_pending_mask |= BIT(completed);
            desc->rx_dma_index ^= 1;
        }
    }
}

_attribute_ram_code_sec_noinline_ void uart0_irq_handler(void)
{
    UART_Desc desc = _get_uart_desc(UART0);

    if (desc == NULL)
        return;

    if (uart_get_irq_status(UART0, UART_TXDONE_IRQ_STATUS))
    {
        uart_clr_irq_status(UART0, UART_TXDONE_IRQ_STATUS);

        if (desc->flags & UF_TX_BUSY)
        {
            desc->flags &= ~UF_TX_BUSY;
            desc->flags |= UF_TX_COMPLETE_PENDING;
        }
    }

    if (uart_get_irq_status(UART0, UART_RX_ERR))
    {
        uart_clr_irq_status(UART0, UART_RXBUF_IRQ_STATUS);
        desc->flags |= UF_RX_ERROR;
    }
}

PLIC_ISR_REGISTER(uart0_irq_handler, IRQ_UART0)

void UARTS_Init(void)
{
    core_interrupt_disable();

    for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
    {
        UART_Desc desc = &uart_desc[i];
        unsigned short div;
        unsigned char bwpc;

        uart_hw_fsm_reset(desc->uart);
        uart_set_pin(desc->uart, desc->pin_tx, desc->pin_rx);
        uart_cal_div_and_bwpc(desc->baudrate,
                              sys_clk.pclk * 1000 * 1000,
                              &div, &bwpc);

        uart_init(desc->uart, div, bwpc,
                  UART_PARITY_NONE, UART_STOP_BIT_ONE);

        /*
         * TL721X RX timeout: 14 bit-times * 2^4 / 115200 ~= 1.94 ms.
         * The timeout is measured from the last received byte.
         */
        uart_set_rx_timeout_with_exp(desc->uart, bwpc,
                                     14, UART_BW_MUL1, 4);

        uart_set_tx_dma_config(desc->uart, desc->dma_tx);
        uart_set_irq_mask(desc->uart,
                          UART_TXDONE_MASK | UART_ERR_IRQ_MASK);

        /*
         * 1024-byte physical buffer = 4-byte hardware length field +
         * 1020-byte DMA payload.
         */
        uart_set_dma_chain_llp(desc->uart, desc->dma_rx,
                               rx_dma_buffer[0] + UART_RX_DMA_DATA_OFFSET,
                               UART_RX_DMA_DATA_SIZE,
                               &rx_dma_list[0]);

        uart_rx_dma_add_list_element(desc->uart, desc->dma_rx,
                                     &rx_dma_list[0], &rx_dma_list[1],
                                     rx_dma_buffer[1] + UART_RX_DMA_DATA_OFFSET,
                                     UART_RX_DMA_DATA_SIZE);

        uart_rx_dma_add_list_element(desc->uart, desc->dma_rx,
                                     &rx_dma_list[1], &rx_dma_list[0],
                                     rx_dma_buffer[0] + UART_RX_DMA_DATA_OFFSET,
                                     UART_RX_DMA_DATA_SIZE);

        dma_clr_tc_irq_status(BIT(desc->dma_rx));
        dma_set_llp_irq_mode(desc->dma_rx, DMA_INTERRUPT_MODE);
        dma_set_irq_mask(desc->dma_rx, TC_MASK);
        dma_chn_en(desc->dma_rx);

        desc->ring = RingBuffer_Init(rx_ring_buffer,
                                     sizeof(rx_ring_buffer));
        desc->flags = 0;
        desc->rx_pending_mask = 0;
        desc->rx_pending_size[0] = 0;
        desc->rx_pending_size[1] = 0;
        desc->rx_dma_index = 0;
        DMA_RegisterCallback(desc->dma_rx, uart_dma_rx_callback);
    }

    plic_interrupt_enable(IRQ_UART0);
    core_interrupt_enable();
}

bool UART_Set_cb(uart_num_e UART, UART_cb cb)
{
    UART_Desc uart = _get_uart_desc(UART);
    if (uart)
    {
        uart->cb = cb;
        return true;
    }
    return false;
}

bool UART_Send(uart_num_e UART, void *data, int length)
{
    UART_Desc uart = _get_uart_desc(UART);

    if ((uart == NULL) || (data == NULL) ||
        (length <= 0) || (length > UART_TX_BUFFER_SIZE))
        return false;

    int int_en = core_interrupt_disable();

    /* Fully non-blocking: do not wait for an active DMA transmission. */
    if (uart->flags & UF_TX_BUSY)
    {
        core_restore_interrupt(int_en);
        return false;
    }

    /*
     * The caller's buffer may disappear immediately after return, so DMA
     * always reads from driver-owned RAM.
     */
    memcpy(uart->tx_buffer, data, length);
    uart->flags |= UF_TX_BUSY;
    core_restore_interrupt(int_en);

    if (uart->cb)
        uart->cb(uart->uart, MESSAGE_UART_TX_START, 0, 0);
    else
        Message_Add(MESSAGE_UART_TX_START, 0, 0, 0);

    uart_send_dma(uart->uart, uart->tx_buffer, length);
    return true;
}

bool UART_SendStr(uart_num_e UART, void *str)
{
    if (str == NULL)
        return false;
    return UART_Send(UART, str, strlen(str));
}

void UART_Poll(void)
{
    for (int index = 0; index < ARRAY_SIZE(uart_desc); index++)
    {
        UART_Desc uart = &uart_desc[index];

        int int_en = core_interrupt_disable();
        uint8_t rx_pending = uart->rx_pending_mask;
        bool tx_complete =
            (uart->flags & UF_TX_COMPLETE_PENDING) != 0;
        bool rx_error =
            (uart->flags & UF_RX_ERROR) != 0;

        uart->rx_pending_mask &= (uint8_t)~rx_pending;
        uart->flags &= ~(UF_TX_COMPLETE_PENDING | UF_RX_ERROR);
        core_restore_interrupt(int_en);

        if (rx_error)
        {
            /* Rebuild the LLP chain outside interrupt context. */
            dma_chn_dis(uart->dma_rx);
            dma_clr_tc_irq_status(BIT(uart->dma_rx));

            uart_set_dma_chain_llp(uart->uart, uart->dma_rx,
                                   rx_dma_buffer[0] + UART_RX_DMA_DATA_OFFSET,
                                   UART_RX_DMA_DATA_SIZE,
                                   &rx_dma_list[0]);

            uart_rx_dma_add_list_element(uart->uart, uart->dma_rx,
                                         &rx_dma_list[0], &rx_dma_list[1],
                                         rx_dma_buffer[1] + UART_RX_DMA_DATA_OFFSET,
                                         UART_RX_DMA_DATA_SIZE);

            uart_rx_dma_add_list_element(uart->uart, uart->dma_rx,
                                         &rx_dma_list[1], &rx_dma_list[0],
                                         rx_dma_buffer[0] + UART_RX_DMA_DATA_OFFSET,
                                         UART_RX_DMA_DATA_SIZE);

            uart->rx_pending_mask = 0;
            uart->rx_pending_size[0] = 0;
            uart->rx_pending_size[1] = 0;
            uart->rx_dma_index = 0;

            dma_set_llp_irq_mode(uart->dma_rx, DMA_INTERRUPT_MODE);
            dma_set_irq_mask(uart->dma_rx, TC_MASK);
            dma_chn_en(uart->dma_rx);
        }

        /*
         * Copy completed DMA buffers into the existing packet ring first.
         * This releases the DMA buffers before any application callback can
         * take a long time.
         */
        for (int buffer_index = 0; buffer_index < 2; buffer_index++)
        {
            if (rx_pending & BIT(buffer_index))
            {
                uint16_t length = uart->rx_pending_size[buffer_index];

                if (length)
                {
                    RingBuffer_AddData(uart->ring,
                                       rx_dma_buffer[buffer_index] +
                                       UART_RX_DMA_DATA_OFFSET,
                                       length);
                }
            }
        }

        /* Never call RX callback from the DMA ISR. */
        if (rx_pending && uart->cb && RingBuffer_GetSize(uart->ring))
        {
            uart->cb(uart->uart,
                     MESSAGE_UART_RECEIVE_DATA,
                     NULL,
                     RingBuffer_GetSize(uart->ring));
        }

        /* Never call TX-complete callback from the UART ISR. */
        if (tx_complete)
        {
            if (uart->cb)
                uart->cb(uart->uart,
                         MESSAGE_UART_TX_COMPLETE, 0, 0);
            else
                Message_Add(MESSAGE_UART_TX_COMPLETE,
                            uart->uart, 0, 0);
        }
    }
}

uint16_t UART_GetRXCount(uart_num_e UART)
{
    UART_Desc desc = _get_uart_desc(UART);
    if (desc)
        return RingBuffer_GetSize(desc->ring);
    return 0;
}

uint16_t UART_Get(uart_num_e UART, void *buffer, uint16_t buffer_count)
{
    UART_Desc desc = _get_uart_desc(UART);
    if (desc)
    {
        return RingBuffer_GetData(desc->ring, buffer, buffer_count);
    }
    return 0;
}
/*
_attribute_ram_code_sec_ void uart_irq_handler(uart_num_e UART)
{
    UART_Desc desc = _get_uart_desc(UART);
    if (desc == NULL)
        return;
    if (uart_get_irq_status(UART, UART_RX_ERR))
    {
        uart_clr_irq_status(UART, UART_RXBUF_IRQ_STATUS); // it will clear rx_fifo,clear hardware pointer and rx_err_irq ,rx_buff_irq,so it won't enter rx_buff_irq interrupt.
    }
    if ((uart_get_irq_status(UART, UART_RXBUF_IRQ_STATUS) != 0) ||
    	(uart_get_irq_status(UART, UART_RXDONE_IRQ_STATUS) != 0))
    {
    	unsigned char fifo_cnt = uart_get_rxfifo_num(UART);
    	for (int i = i; i < fifo_cnt; i++)
    	{
    		RingBuffer_WriteByte(desc->buffer, uart_read_byte(UART));
    	}
    	desc->flags |= UF_RX_DONE;
    }
}

_attribute_ram_code_sec_ void uart0_irq_handler(void)
{
    uart_irq_handler(UART0);
}

_attribute_ram_code_sec_ void uart1_irq_handler(void)
{
    uart_irq_handler(UART1);
}

_attribute_ram_code_sec_ void uart2_irq_handler(void)
{
    uart_irq_handler(UART2);
}

PLIC_ISR_REGISTER(uart0_irq_handler, IRQ_UART0)
PLIC_ISR_REGISTER(uart1_irq_handler, IRQ_UART1)
PLIC_ISR_REGISTER(uart2_irq_handler, IRQ_UART2)
*/
