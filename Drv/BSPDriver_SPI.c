#include "BSPDriver_SPI.h"
#include <stdbool.h>
#include <string.h>
#include "gpio.h"
#include "../def.h"

/*
 * DMA buffers used by the Telink SPI driver must be word aligned.  The
 * public API therefore rejects unaligned buffers instead of silently
 * allowing a DMA fault or a write beyond a short RX buffer.
 */
#define SPI_DMA_ALIGNMENT      4u
#define SPI_DMA_ALIGNMENT_MASK (SPI_DMA_ALIGNMENT - 1u)
#define SPI_BLOCKING_TX_MAX    64u

lspi_pin_config_t lspi_pin_config =
{
    .spi_csn_pin      = (lspi_pin_def_e)GPIO_NONE_PIN,
    .spi_clk_pin      = LSPI_CLK_PE1_PIN,
    .spi_mosi_io0_pin = LSPI_MOSI_IO0_PE2_PIN,
    .spi_miso_io1_pin = LSPI_MISO_IO1_PE3_PIN,
    .spi_io2_pin      = LSPI_IO2_PE4_PIN,
    .spi_io3_pin      = LSPI_IO3_PE5_PIN,
};

gspi_pin_config_t gspi_pin_config =
{
    .spi_clk_pin      = GPIO_PB2,
    .spi_csn_pin      = GPIO_NONE_PIN,
    .spi_mosi_io0_pin = GPIO_PB6,
    .spi_miso_io1_pin = GPIO_PB5,
    .spi_io2_pin      = GPIO_PB4,
    .spi_io3_pin      = GPIO_PB3
};

typedef enum
{
    SPI_TRANSFER_NONE = 0,
    SPI_TRANSFER_WRITE,
    SPI_TRANSFER_READ
} BSP_DRIVER_SPI_TRANSFER;

typedef struct
{
    spi_sel_e module;
    spi_mode_type_e mode;
    void *pin_config;
    spi_wr_rd_config_t *config;
    dma_chn_e transmit_dma_channel;
    dma_chn_e receive_dma_channel;
    uint32_t speed;
    BSP_DRIVER_SPI_CB cb;
    volatile BSP_DRIVER_SPI_TRANSFER transfer;
    volatile bool dma_done;
    volatile bool spi_done;
    volatile bool busy;
    volatile bool notify;
} BSP_DRIVER_SPI_Def;

static spi_wr_rd_config_t lspi_config =
{
    .spi_io_mode     = SPI_QUAD_MODE,
    .spi_dummy_cnt   = 0,
    .spi_cmd_en      = 0,
    .spi_addr_en     = 0,
    .spi_addr_len    = 0,
    .spi_cmd_fmt_en  = 0,
    .spi_addr_fmt_en = 0,
};

static spi_wr_rd_config_t gspi_config =
{
    .spi_io_mode     = SPI_3_LINE_MODE,
    .spi_dummy_cnt   = 0,
    .spi_cmd_en      = 0,
    .spi_addr_en     = 0,
    .spi_addr_len    = 0,
    .spi_cmd_fmt_en  = 0,
    .spi_addr_fmt_en = 0,
};

static BSP_DRIVER_SPI_Def spi[BDSID_COUNT] =
{
    {
        LSPI_MODULE,
        SPI_MODE3,
        &lspi_pin_config,
        &lspi_config,
        DMA0,
        DMA1,
        12000000,
        NULL,
        SPI_TRANSFER_NONE,
        false,
        false,
        false,
        false
    },
    {
        GSPI_MODULE,
        SPI_MODE3,
        &gspi_pin_config,
        &gspi_config,
        DMA2,
        DMA3,
        1000000,
        NULL,
        SPI_TRANSFER_NONE,
        false,
        false,
        false,
        false
    }
};

static bool LSPI_sleep = false;

static bool _spi_valid_id(BSP_DRIVER_SPI_ID ID)
{
    return ID < BDSID_COUNT;
}

static bool _spi_valid_dma_buffer(void *buffer, uint32_t count)
{
    if (buffer == NULL || count == 0)
        return false;

    if ((((uint32_t)buffer) & SPI_DMA_ALIGNMENT_MASK) != 0)
        return false;

    /* Telink SPI DMA transfers data in 32-bit units. */
    if ((count & SPI_DMA_ALIGNMENT_MASK) != 0)
        return false;

    return true;
}

static void _spi_reset_transfer(BSP_DRIVER_SPI_ID ID)
{
    spi[ID].transfer = SPI_TRANSFER_NONE;
    spi[ID].dma_done = false;
    spi[ID].spi_done = false;
    spi[ID].busy = false;
    spi[ID].notify = false;
}

static void _spi_complete(BSP_DRIVER_SPI_ID ID)
{
    BSP_DRIVER_SPI_CB cb;
    BSP_DRIVER_SPI_MSG msg;

    if (!spi[ID].busy || !spi[ID].dma_done || !spi[ID].spi_done)
        return;

    /* SPI_END_INT is only the end of FIFO activity. The SDK requires
     * waiting until the peripheral itself reports idle. */
    if (spi_is_busy(spi[ID].module))
        return;

    msg = (spi[ID].transfer == SPI_TRANSFER_READ) ? BDSM_READEN : BDSM_WRITEN;
    cb = spi[ID].notify ? spi[ID].cb : NULL;

    spi[ID].transfer = SPI_TRANSFER_NONE;
    spi[ID].dma_done = false;
    spi[ID].spi_done = false;
    spi[ID].busy = false;
    spi[ID].notify = false;

    if (cb)
        cb(ID, msg);
}

_attribute_ram_code_sec_noinline_ void bsp_driver_spi_irq_handler(void)
{
    int i;

    for (i = 0; i < BDSID_COUNT; i++)
    {
        if (spi_get_irq_status(spi[i].module, SPI_END_INT))
        {
            spi_clr_irq_status(spi[i].module, SPI_END_INT);
            if (spi[i].busy)
            {
                spi[i].spi_done = true;
                _spi_complete((BSP_DRIVER_SPI_ID)i);
            }
        }
    }
}

PLIC_ISR_REGISTER(bsp_driver_spi_irq_handler, IRQ_LSPI)

/*
 * DMA TC is used together with SPI_END_INT.  DMA TC means that the DMA
 * engine has finished moving the buffer; SPI_END_INT + spi_is_busy() is the
 * peripheral-side completion indication.  Requiring both prevents the
 * callback from being issued while the last byte is still on the wire.
 */
_attribute_ram_code_sec_noinline_ void bsp_driver_spi_dma_irq_handler(void)
{
    int i;

    for (i = 0; i < BDSID_COUNT; i++)
    {
        uint32_t dma_mask = BIT(spi[i].transmit_dma_channel) |
                            BIT(spi[i].receive_dma_channel);

        if (dma_get_tc_irq_status(dma_mask))
        {
            dma_clr_tc_irq_status(dma_mask);

            if (spi[i].busy)
            {
                spi[i].dma_done = true;
                _spi_complete((BSP_DRIVER_SPI_ID)i);
            }
        }
    }
}

PLIC_ISR_REGISTER(bsp_driver_spi_dma_irq_handler, IRQ_DMA)

static void _spi_hw_init(BSP_DRIVER_SPI_ID ID)
{
    BSP_DRIVER_SPI_Def *dev = &spi[ID];

    spi_master_init(dev->module,
                    sys_clk.pll_clk * 1000000 / dev->speed,
                    dev->mode);

    switch (ID)
    {
        case BDSID_LSPI:
            lspi_set_pin((lspi_pin_config_t *)dev->pin_config);
            spi_master_config(dev->module,
                              dev->config->spi_io_mode == SPI_3_LINE_MODE ?
                              SPI_3LINE : SPI_NORMAL);
            BSP_DRIVER_SPI_SetMode(ID, dev->config->spi_io_mode);
            break;

        case BDSID_GSPI:
            gspi_set_pin((gspi_pin_config_t *)dev->pin_config);
            spi_master_config(dev->module,
                              dev->config->spi_io_mode == SPI_3_LINE_MODE ?
                              SPI_3LINE : SPI_NORMAL);
            BSP_DRIVER_SPI_SetMode(ID, dev->config->spi_io_mode);
            break;

        default:
            break;
    }

    /* Configure both DMA directions. LSPI RX is not used by the project,
     * but keeping its DMA channel configured makes the controller generic. */
    spi_set_tx_dma_config(dev->module, dev->transmit_dma_channel);
    spi_set_master_rx_dma_config(dev->module, dev->receive_dma_channel);

    /* Clear stale completion state before enabling interrupts. */
    spi_clr_irq_status(dev->module, SPI_END_INT);
    spi_set_irq_mask(dev->module, SPI_END_INT_EN);

    dma_clr_tc_irq_status(BIT(dev->transmit_dma_channel) |
                          BIT(dev->receive_dma_channel));
    dma_set_irq_mask(dev->transmit_dma_channel, TC_MASK);
    dma_set_irq_mask(dev->receive_dma_channel, TC_MASK);
}

void BSP_DRIVER_SPI_Init(void)
{
    int i;

    for (i = 0; i < BDSID_COUNT; i++)
    {
        _spi_reset_transfer((BSP_DRIVER_SPI_ID)i);
        _spi_hw_init((BSP_DRIVER_SPI_ID)i);
    }

    plic_interrupt_enable(IRQ_DMA);
    plic_interrupt_enable(IRQ_LSPI);
}

void BSP_DRIVER_SPI_SetCallback(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_CB cb)
{
    if (_spi_valid_id(ID))
        spi[ID].cb = cb;
}

void BSP_DRIVER_SPI_SetMode(BSP_DRIVER_SPI_ID ID, spi_io_mode_e mode)
{
    if (_spi_valid_id(ID))
        spi_set_io_mode(spi[ID].module, mode);
}

static int _spi_start(BSP_DRIVER_SPI_ID ID,
                      BSP_DRIVER_SPI_TRANSFER transfer,
                      void *buffer,
                      uint32_t count,
                      bool notify)
{
    if (!_spi_valid_id(ID) || !_spi_valid_dma_buffer(buffer, count))
        return BDSM_ERROR;

    if (spi[ID].busy)
        return BDSM_TAKEN;

    spi[ID].transfer = transfer;
    spi[ID].dma_done = false;
    spi[ID].spi_done = false;
    spi[ID].busy = true;
    spi[ID].notify = notify;

    dma_clr_tc_irq_status(BIT(spi[ID].transmit_dma_channel) |
                          BIT(spi[ID].receive_dma_channel));

    if (transfer == SPI_TRANSFER_WRITE)
        spi_master_write_dma(spi[ID].module, (unsigned char *)buffer, count);
    else
        spi_master_read_dma(spi[ID].module, (unsigned char *)buffer, count);

    return BDSM_OK;
}

int BSP_DRIVER_SPI_Write(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
    return _spi_start(ID, SPI_TRANSFER_WRITE, buffer, count, true);
}

int BSP_DRIVER_SPI_Read(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
    /* LSPI is intentionally TX-only in this project. */
    if (ID == BDSID_LSPI)
        return BDSM_ERROR;

    return _spi_start(ID, SPI_TRANSFER_READ, buffer, count, true);
}

int BSP_DRIVER_SPI_WriteBlocking(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
    static uint8_t tx_buffer[BDSID_COUNT][SPI_BLOCKING_TX_MAX] __attribute__((aligned(4)));
    uint32_t dma_count;
    int result;

    if (!_spi_valid_id(ID) || buffer == NULL || count == 0 ||
        count > SPI_BLOCKING_TX_MAX)
        return BDSM_ERROR;

    dma_count = (count + SPI_DMA_ALIGNMENT_MASK) & ~SPI_DMA_ALIGNMENT_MASK;
    memcpy(tx_buffer[ID], buffer, count);
    if (dma_count > count)
        memset(tx_buffer[ID] + count, 0, dma_count - count);

    result = _spi_start(ID, SPI_TRANSFER_WRITE, tx_buffer[ID], dma_count, false);
    if (result != BDSM_OK)
        return result;

    /* Intended for normal application context, not an ISR. */
    while (spi[ID].busy)
    {
        /* The completion is interrupt driven; no polling is required here. */
    }

    return BDSM_OK;
}

void BSP_DRIVER_SPI_Poll(void)
{
    /* Compatibility stub. Completion is now interrupt driven. */
}

static void _disable_pin(gpio_pin_e pin)
{
    if (pin == GPIO_NONE_PIN)
        return;

    gpio_set_up_down_res(pin, GPIO_PIN_UP_DOWN_FLOAT);
    gpio_set_mux_function(pin, AS_GPIO);
    gpio_function_en(pin);
    gpio_set_low_level(pin);
    gpio_output_en(pin);
    gpio_input_dis(pin);
}

void BSP_DRIVER_SPI_Sleep(BSP_DRIVER_SPI_ID ID)
{
    if (!_spi_valid_id(ID))
        return;

    if (spi[ID].busy)
        return;

    switch (ID)
    {
        case BDSID_LSPI:
            spi_hw_fsm_reset(spi[ID].module);
            reg_clk_en0 &= ~FLD_CLK0_LSPI_EN;
            _disable_pin(lspi_pin_config.spi_clk_pin);
            _disable_pin(lspi_pin_config.spi_csn_pin);
            _disable_pin(lspi_pin_config.spi_io2_pin);
            _disable_pin(lspi_pin_config.spi_io3_pin);
            _disable_pin(lspi_pin_config.spi_miso_io1_pin);
            _disable_pin(lspi_pin_config.spi_mosi_io0_pin);
            LSPI_sleep = true;
            break;

        case BDSID_GSPI:
            /* GSPI sleep clock control is not changed here until the exact
             * TL7218AE clock-gate definition is verified. */
            break;

        default:
            break;
    }
}

void BSP_DRIVER_SPI_Wakeup(BSP_DRIVER_SPI_ID ID)
{
    if (!_spi_valid_id(ID))
        return;

    switch (ID)
    {
        case BDSID_LSPI:
            _spi_hw_init(ID);
            LSPI_sleep = false;
            break;

        case BDSID_GSPI:
            _spi_hw_init(ID);
            break;

        default:
            break;
    }
}

bool BSP_DRIVER_SPI_IsSleep(BSP_DRIVER_SPI_ID ID)
{
    switch (ID)
    {
        case BDSID_LSPI:
            return LSPI_sleep;
        case BDSID_GSPI:
            return false;
        default:
            return false;
    }
}
