#include "tl_common.h"
#include "BSP_dma.h"

static BSP_DMA_Callback dma_callbacks[BSP_DMA_MAX_CHANNELS];
static bool dma_initialized = false;

static bool _dma_valid_channel(dma_chn_e channel)
{
    return (unsigned int)channel < BSP_DMA_MAX_CHANNELS;
}

_attribute_ram_code_sec_noinline_ void bsp_dma_irq_handler(void)
{
    for (unsigned int channel = 0; channel < BSP_DMA_MAX_CHANNELS; ++channel)
    {
        if (dma_callbacks[channel] != NULL &&
            dma_get_tc_irq_status(BIT(channel)))
        {
            dma_clr_tc_irq_status(BIT(channel));
            dma_callbacks[channel]((dma_chn_e)channel);
        }
    }
}

PLIC_ISR_REGISTER(bsp_dma_irq_handler, IRQ_DMA)

void BSP_DMA_Init(void)
{
    if (!dma_initialized)
    {
        plic_interrupt_enable(IRQ_DMA);
        dma_initialized = true;
    }
}

bool BSP_DMA_RegisterCallback(dma_chn_e channel, BSP_DMA_Callback callback)
{
    if (!_dma_valid_channel(channel) || callback == NULL)
        return false;

    BSP_DMA_Init();
    dma_callbacks[channel] = callback;
    return true;
}
