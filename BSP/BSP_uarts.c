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

typedef enum
{
	UF_TX_BUSY = BIT(0),
	UF_TX_RUN = BIT(1),
	UF_RX_DONE = BIT(2),
	UF_ERROR = BIT(3),
/*
	UF_LED1 = BIT(5),
	UF_LED2 = BIT(6),
	UF_LED3 = BIT(7),
*/
}UART_Flags;

typedef struct
{
	uart_num_e uart;
	gpio_pin_e pin_tx;
	gpio_pin_e pin_rx;
	dma_chn_e dma_tx;
	unsigned int baudrate;
	unsigned int plic;
	UART_cb cb;
	Handle rx_buffer;
	Handle tx_buffer;
	uint16_t tx_buffer_size;
	uint16_t rx_data_size;
	uint16_t rx_timeout;
	TimerHandle rx_timer;
	UART_Flags flags;
	Handle ring;
	uint16_t rx_tmp_buffer_count;
	uint8_t buffer[UART_TX_BUFFER_SIZE];
	uint8_t rx_tmp_buffer[UART_RX_TMP_BUFFER_SIZE];
}UART_Desc_t;

typedef UART_Desc_t *UART_Desc;

static UART_Desc_t uart_desc[] =
{
		{
				.uart = UART0,
				.pin_rx = GPIO_PA3,// GPIO_PA4,//GPIO_PC5, //
				.pin_tx = GPIO_NONE_PIN,//GPIO_PA3,
				.baudrate = UART_DEFAULT_BAUDRATE,
				.dma_tx = DMA4,
				.plic = IRQ_UART0,
				.cb = NULL,
				.rx_buffer = NULL,
				.tx_buffer = NULL,
				.tx_buffer_size = 0,
				.rx_data_size = 0,
				.rx_timeout = MS2COUNT(UART_DEFAULT_TIMEOUT),
				.rx_timer = NULL,
				.flags = 0,
				.ring = NULL,
				.rx_tmp_buffer_count = 0
		}
};

static uint8_t rx_buffer[ARRAY_SIZE(uart_desc) * UART_RX_BUFFER_SIZE];

static UART_Desc _get_uart_desc(uart_num_e uart)
{
	UART_Desc result = NULL;
	for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
	{
		if (uart_desc[i].uart == uart)
		{
			result = &uart_desc[i];
			break;
		}
	}
	return result;
}

static UART_Desc _get_uart_desc_from_dma(dma_chn_e channel)
{
	UART_Desc result = NULL;
	for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
	{
		if (uart_desc[i].dma_tx == channel)
		{
			result = &uart_desc[i];
			break;
		}
	}
	return result;
}

static UART_Desc _get_uart_desc_from_timer(TimerHandle timer)
{
	UART_Desc result = NULL;
	for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
	{
		if (uart_desc[i].rx_timer == timer)
		{
			result = &uart_desc[i];
			break;
		}
	}
	return result;
}

static void UART_DMAcb(dma_chn_e channel, dma_irq_mask_e mask)
{
	UART_Desc desc = _get_uart_desc_from_dma(channel);
	if (mask == TC_MASK)
	{
		desc->flags &= (~UF_TX_RUN);
	}
	if (mask == ERR_MASK)
	{
		desc->flags |= UF_ERROR;
	}
}

static void UART_Timercb(TimerHandle timer)
{
	UART_Desc desc = _get_uart_desc_from_timer(timer);
    if (desc->rx_tmp_buffer_count)
    {
    	//tlk_printf("Recv: %d\n", desc->rx_tmp_buffer_count);
    	RingBuffer_AddData(desc->ring, desc->rx_tmp_buffer, desc->rx_tmp_buffer_count);
    	desc->rx_tmp_buffer_count = 0;
    }
	uart_hw_fsm_reset(desc->uart);
}

void UARTS_Init(void)
{
	core_interrupt_disable();
	for (int i = 0; i < ARRAY_SIZE(uart_desc); i++)
	{
		unsigned short div;
		unsigned char  bwpc;
		uint16_t rx_buffer_len = UART_RX_BUFFER_SIZE - 1;
		uart_hw_fsm_reset(uart_desc[i].uart);
		uart_set_pin(uart_desc[i].uart, uart_desc[i].pin_tx, uart_desc[i].pin_rx);
		uart_cal_div_and_bwpc(uart_desc[i].baudrate, sys_clk.pclk * 1000 * 1000, &div, &bwpc);
		uart_init(uart_desc[i].uart, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);
		//uart_set_tx_dma_config(uart_desc[i].uart, uart_desc[i].dma_tx);
		//uart_set_irq_mask(uart_desc[i].uart, UART_TXDONE_MASK | UART_RX_IRQ_MASK);
		//plic_interrupt_enable(uart_desc[i].plic);
		uart_desc[i].tx_buffer_size = UART_TX_BUFFER_SIZE;
		//BSP_DMARegister(uart_desc[i].dma_tx, UART_DMAcb, BIT(TC_MASK) | BIT(ERR_MASK));
		uart_desc[i].rx_timer = SoftTimers_Create(uart_desc[i].rx_timeout, false, UART_Timercb);
		uart_desc[i].ring = RingBuffer_Init(&(rx_buffer[i * UART_RX_BUFFER_SIZE]), UART_RX_BUFFER_SIZE);
		uart_desc[i].rx_tmp_buffer_count = 0;
	}
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
	if ((uart != NULL) && (data != NULL) && (length > 0) && (length <= uart->tx_buffer_size))
	{
		while (uart->flags & (UF_TX_BUSY | UF_ERROR))
			UART_Poll();
		//memcpy(uart->tx_buffer, data, length);
		if (uart->cb)
			uart->cb(uart->uart, MESSAGE_UART_TX_START, 0, 0);
		else
			Message_Add(MESSAGE_UART_TX_START, 0, 0, 0);
		uart->flags |= UF_TX_BUSY;// | UF_TX_RUN | UF_LED3;
		//uart_send_dma(uart->uart, uart->tx_buffer, length);
		uint16_t count = 0;
		uint8_t *buf = data;
		while (length)
		{
			uint8_t count;
			if (length > 255)
				count = 255;
			else
				count = length;
			uart_send(uart->uart, buf, count);
			buf += count;
			length -= count;
		}
		return true;
	}
	return false;
}

bool UART_SendStr(uart_num_e UART, void *str)
{
	UART_Send(UART, str, strlen(str));
}

static uint8_t uart_index = 0;
static bool polling_en = true;

void UART_Poll(void)
{
	if (uart_index >= ARRAY_SIZE(uart_desc))
		uart_index = 0;
	UART_Desc uart = &(uart_desc[uart_index]);
	int int_en = core_interrupt_disable();
	UART_Flags flags = uart->flags;
	core_restore_interrupt(int_en);
	//NEURO_LED1(((flags & UF_LED1) != 0) ? ON : OFF);
	//NEURO_LED2(((flags & UF_LED2) != 0) ? ON : OFF);
	//NEURO_LED3(((flags & UF_LED3) != 0) ? ON : OFF);
	if (flags & UF_ERROR)
	{
		uart_hw_fsm_reset(uart->uart);
		flags = UF_TX_BUSY | UF_TX_RUN | UF_ERROR;
		tlk_printf("UART error\n");
	}
	else
	{
		if (flags & UF_TX_BUSY)
		{
			if ((flags & UF_TX_RUN) == 0)
			{

				flags = UF_TX_BUSY;
				if (uart->cb)
					uart->cb(uart->uart, MESSAGE_UART_TX_COMPLETE, 0, 0);
				else
					Message_Add(MESSAGE_UART_TX_COMPLETE, uart->uart, 0, 0);
			}
		}
		else
			flags = 0;
		unsigned char fifo_cnt = uart_get_rxfifo_num(uart->uart);

		if (fifo_cnt)
		{
			while (fifo_cnt--)
			{
				if (uart->rx_tmp_buffer_count >= sizeof(uart->rx_tmp_buffer))
				{
					uart->rx_tmp_buffer_count = 0;
					tlk_printf("UART ovfl\n");
				}

				uart->rx_tmp_buffer[uart->rx_tmp_buffer_count++] = uart_read_byte(uart->uart);
			}
			SoftTimers_Restart(uart->rx_timer, uart->rx_timeout);
		}
	/*
		if (uart_desc[uart_index].flags & UF_RX_DONE)
		{
			SoftTimers_Restart(uart_desc[uart_index].rx_timer, uart_desc[uart_index].rx_timeout);
			uart_desc[uart_index].flags &= (~UF_RX_DONE);
		}
*/
	}
	if (RingBuffer_GetSize(uart->ring))
	{
		if (uart->cb)
			uart->cb(uart->uart, MESSAGE_UART_RECEIVE_DATA, NULL, RingBuffer_GetSize(uart->ring));
	}
	int_en = core_interrupt_disable();
	uart->flags &= ~flags;
	core_restore_interrupt(int_en);
	uart_index++;
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
