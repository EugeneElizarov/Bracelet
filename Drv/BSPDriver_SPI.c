#include "BSPDriver_SPI.h"
#include <stdbool.h>
#include <string.h>
#include "gpio.h"
#include "../def.h"

#define SPI_DMA_ALIGNMENT      4u
#define SPI_DMA_ALIGNMENT_MASK (SPI_DMA_ALIGNMENT - 1u)
#define SPI_BLOCKING_TX_MAX    64u
#define SPI_BLOCKING_TIMEOUT_US 10000u

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
    dma_chn_e tx_dma_channel;
    dma_chn_e rx_dma_channel;
    uint32_t speed;
    BSP_DRIVER_SPI_CB cb;
    volatile BSP_DRIVER_SPI_TRANSFER transfer;
    volatile bool busy;
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
        false
    }
};

static bool LSPI_sleep = false;

static bool _spi_valid_id(BSP_DRIVER_SPI_ID ID)
{
    return ID < BDSID_COUNT;
}

static bool _spi_valid_dma_buffer(const void *buffer)
{
    return buffer != NULL &&
           ((((uint32_t)buffer) & SPI_DMA_ALIGNMENT_MASK) == 0);
}

static void _spi_finish(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_MSG msg)
{
    BSP_DRIVER_SPI_CB cb = spi[ID].cb;

    spi[ID].transfer = SPI_TRANSFER_NONE;
    spi[ID].busy = false;

    /*
     * The completion callback is intentionally executed directly from the
     * SPI/DMA ISR.  The current user is lv_disp_flush_ready(), which only
     * marks the LVGL flush as complete and is safe to invoke from the ISR.
     */
    if (cb)
        cb(ID, msg);
}

/*
 * TL721X has separate PLIC sources for LSPI and GSPI.  SPI_END_INT is used
 * for master TX completion, exactly as recommended by the Telink SDK.
 */
_attribute_ram_code_sec_noinline_ void bsp_driver_spi_lspi_irq_handler(void)
{
    if (spi_get_irq_status(LSPI_MODULE, SPI_END_INT))
    {
        spi_clr_irq_status(LSPI_MODULE, SPI_END_INT);

        if (spi[BDSID_LSPI].busy &&
            spi[BDSID_LSPI].transfer == SPI_TRANSFER_WRITE)
        {
            _spi_finish(BDSID_LSPI, BDSM_WRITEN);
        }
    }
}

PLIC_ISR_REGISTER(bsp_driver_spi_lspi_irq_handler, IRQ_LSPI)

_attribute_ram_code_sec_noinline_ void bsp_driver_spi_gspi_irq_handler(void)
{
    if (spi_get_irq_status(GSPI_MODULE, SPI_END_INT))
    {
        spi_clr_irq_status(GSPI_MODULE, SPI_END_INT);

        if (spi[BDSID_GSPI].busy &&
            spi[BDSID_GSPI].transfer == SPI_TRANSFER_WRITE)
        {
            _spi_finish(BDSID_GSPI, BDSM_WRITEN);
        }
    }
}

PLIC_ISR_REGISTER(bsp_driver_spi_gspi_irq_handler, IRQ_GSPI)

/*
 * For master RX the SDK explicitly recommends DMA TC, because SPI_END_INT
 * can indicate that SPI reception ended before DMA has finished writing the
 * destination buffer.
 */
_attribute_ram_code_sec_noinline_ void bsp_driver_spi_dma_irq_handler(void)
{
    int i;

    for (i = 0; i < BDSID_COUNT; ++i)
    {
        if (spi[i].busy &&
            spi[i].transfer == SPI_TRANSFER_READ &&
            dma_get_tc_irq_status(BIT(spi[i].rx_dma_channel)))
        {
            dma_clr_tc_irq_status(BIT(spi[i].rx_dma_channel));
            _spi_finish((BSP_DRIVER_SPI_ID)i, BDSM_READEN);
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

    /*
     * The TL721X SDK defaults the SPI polling timeout to 0xffffffff us.
     * A missing/broken SPI clock or FIFO condition therefore looks like an
     * infinite loop during display initialization.  Keep the driver's
     * blocking path bounded; even the largest blocking display chunk is
     * well below this at 12 MHz.
     */
    spi_set_error_timeout(dev->module, SPI_BLOCKING_TIMEOUT_US);

    if (ID == BDSID_LSPI)
    {
        lspi_set_pin((lspi_pin_config_t *)dev->pin_config);
    }
    else
    {
        gspi_set_pin((gspi_pin_config_t *)dev->pin_config);
    }

    spi_master_config(dev->module,
                      dev->config->spi_io_mode == SPI_3_LINE_MODE ?
                      SPI_3LINE : SPI_NORMAL);
    spi_set_io_mode(dev->module, dev->config->spi_io_mode);

    spi_set_tx_dma_config(dev->module, dev->tx_dma_channel);
    spi_set_master_rx_dma_config(dev->module, dev->rx_dma_channel);

    /*
     * TX completion: SPI_END_INT.
     * RX completion: DMA terminal count.
     * Do not enable SPI_END_INT for RX; the SDK explicitly warns against it.
     */
    spi_clr_irq_status(dev->module, SPI_END_INT);
    spi_set_irq_mask(dev->module, SPI_END_INT_EN);

    dma_clr_tc_irq_status(BIT(dev->tx_dma_channel) |
                          BIT(dev->rx_dma_channel));
    dma_clr_irq_mask(dev->tx_dma_channel, TC_MASK);
    dma_set_irq_mask(dev->rx_dma_channel, TC_MASK);
}

void BSP_DRIVER_SPI_Init(void)
{
    int i;

    for (i = 0; i < BDSID_COUNT; ++i)
    {
        spi[i].transfer = SPI_TRANSFER_NONE;
        spi[i].busy = false;
        spi[i].cb = NULL;
        _spi_hw_init((BSP_DRIVER_SPI_ID)i);
    }

    plic_interrupt_enable(IRQ_DMA);
    plic_interrupt_enable(IRQ_LSPI);
    plic_interrupt_enable(IRQ_GSPI);
}

void BSP_DRIVER_SPI_SetCallback(BSP_DRIVER_SPI_ID ID, BSP_DRIVER_SPI_CB cb)
{
    if (_spi_valid_id(ID))
        spi[ID].cb = cb;
}

void BSP_DRIVER_SPI_SetMode(BSP_DRIVER_SPI_ID ID, spi_io_mode_e mode)
{
    if (_spi_valid_id(ID) && !spi[ID].busy)
        spi_set_io_mode(spi[ID].module, mode);
}

static int _spi_start(BSP_DRIVER_SPI_ID ID,
                      BSP_DRIVER_SPI_TRANSFER transfer,
                      void *buffer,
                      uint32_t count)
{
    BSP_DRIVER_SPI_Def *dev;

    if (!_spi_valid_id(ID) || count == 0 || !_spi_valid_dma_buffer(buffer))
        return BDSM_ERROR;

    dev = &spi[ID];

    if (dev->busy)
        return BDSM_TAKEN;

    /*
     * The SDK's DMA size is specified in bytes, although the DMA engine
     * itself uses WORD transfers. The SDK handles the final partial word;
     * the only buffer restriction for these APIs is word alignment.
     */
    dma_clr_tc_irq_status(BIT(dev->tx_dma_channel) |
                          BIT(dev->rx_dma_channel));

    dev->transfer = transfer;
    dev->busy = true;

    if (transfer == SPI_TRANSFER_WRITE)
    {
        /*
         * TX completion is reported by SPI_END_INT. No TX DMA interrupt is
         * needed and therefore the TX TC mask remains disabled.
         */
        spi_set_irq_mask(dev->module, SPI_END_INT_EN);
        spi_master_write_dma(dev->module, (unsigned char *)buffer, count);
    }
    else
    {
        /*
         * Telink explicitly requires SPI_END_INT to be disabled during
         * master RX and DMA TC to be used as the completion indication.
         */
        spi_clr_irq_mask(dev->module, SPI_END_INT_EN);
        spi_master_read_dma_plus(dev->module,
                                 0,
                                 0,
                                 (unsigned char *)buffer,
                                 count,
                                 SPI_MODE_RD_READ_ONLY);
    }

    return BDSM_OK;
}

int BSP_DRIVER_SPI_Write(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
    return _spi_start(ID, SPI_TRANSFER_WRITE, buffer, count);
}

int BSP_DRIVER_SPI_Read(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
    if (!_spi_valid_id(ID) || ID == BDSID_LSPI)
        return BDSM_ERROR;

    return _spi_start(ID, SPI_TRANSFER_READ, buffer, count);
}

int BSP_DRIVER_SPI_WriteBlocking(BSP_DRIVER_SPI_ID ID, void *buffer, uint32_t count)
{
    BSP_DRIVER_SPI_Def *dev;

    if (!_spi_valid_id(ID) || buffer == NULL || count == 0)
        return BDSM_ERROR;

    dev = &spi[ID];

    if (dev->busy)
        return BDSM_TAKEN;

    /*
     * Use Telink's native blocking master-write API for short transactions.
     *
     * SPI_END_INT is used by the asynchronous DMA path.  It must be masked
     * during a blocking transaction: spi_master_write() also completes a
     * normal SPI transaction and otherwise its SPI_END status could enter
     * our ISR while the driver is still marked busy, falsely reporting the
     * command transfer as the asynchronous frame completion.
     *
     * This is especially important for the display driver: command writes
     * and the pixel DMA transfer share the same CS window.
     */
    spi_clr_irq_mask(dev->module, SPI_END_INT_EN);
    spi_clr_irq_status(dev->module, SPI_END_INT);

    dev->busy = true;
    dev->transfer = SPI_TRANSFER_NONE;

    drv_api_status_e status = spi_master_write(dev->module,
                                               (unsigned char *)buffer,
                                               count);

    spi_clr_irq_status(dev->module, SPI_END_INT);
    spi_set_irq_mask(dev->module, SPI_END_INT_EN);

    dev->transfer = SPI_TRANSFER_NONE;
    dev->busy = false;

    return status == DRV_API_SUCCESS ? BDSM_OK : BDSM_ERROR;
}

void BSP_DRIVER_SPI_Poll(void)
{
    /* Completion callbacks are delivered by the SPI peripheral ISR. */
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
    BSP_DRIVER_SPI_Def *dev;

    if (!_spi_valid_id(ID))
        return;

    dev = &spi[ID];

    if (dev->busy)
        return;

    spi_hw_fsm_reset(dev->module);

    if (ID == BDSID_LSPI)
    {
        reg_clk_en0 &= ~FLD_CLK0_LSPI_EN;
        _disable_pin(lspi_pin_config.spi_clk_pin);
        _disable_pin(lspi_pin_config.spi_csn_pin);
        _disable_pin(lspi_pin_config.spi_io2_pin);
        _disable_pin(lspi_pin_config.spi_io3_pin);
        _disable_pin(lspi_pin_config.spi_miso_io1_pin);
        _disable_pin(lspi_pin_config.spi_mosi_io0_pin);
        LSPI_sleep = true;
    }
    else
    {
        reg_clk_en1 &= ~FLD_CLK1_GSPI_EN;
        _disable_pin(gspi_pin_config.spi_clk_pin);
        _disable_pin(gspi_pin_config.spi_csn_pin);
        _disable_pin(gspi_pin_config.spi_io2_pin);
        _disable_pin(gspi_pin_config.spi_io3_pin);
        _disable_pin(gspi_pin_config.spi_miso_io1_pin);
        _disable_pin(gspi_pin_config.spi_mosi_io0_pin);
    }
}

void BSP_DRIVER_SPI_Wakeup(BSP_DRIVER_SPI_ID ID)
{
    if (!_spi_valid_id(ID))
        return;

    _spi_hw_init(ID);

    if (ID == BDSID_LSPI)
        LSPI_sleep = false;
}

bool BSP_DRIVER_SPI_IsSleep(BSP_DRIVER_SPI_ID ID)
{
    if (ID == BDSID_LSPI)
        return LSPI_sleep;

    return false;
}
