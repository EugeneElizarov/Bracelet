#include "DMA.h"

#define DMA_CALLBACK_COUNT 8

static DMA_Callback dma_callbacks[DMA_CALLBACK_COUNT];
static bool dma_initialized = false;

static bool dma_channel_valid(dma_chn_e channel)
{
    return (unsigned int)channel < DMA_CALLBACK_COUNT;
}

_attribute_ram_code_sec_noinline_ static void dma_irq_handler(void)
{
    unsigned int channel;

    for (channel = 0; channel < DMA_CALLBACK_COUNT; ++channel)
    {
        DMA_Callback cb = dma_callbacks[channel];

        if (cb == NULL)
            continue;

        if (dma_get_tc_irq_status(BIT(channel)))
        {
            dma_clr_tc_irq_status(BIT(channel));
            cb((dma_chn_e)channel);
        }
    }
}

PLIC_ISR_REGISTER(dma_irq_handler, IRQ_DMA)

void DMA_Init(void)
{
    if (dma_initialized)
        return;

    dma_initialized = true;
    plic_interrupt_enable(IRQ_DMA);
}

bool DMA_RegisterCallback(dma_chn_e channel, DMA_Callback cb)
{
    int int_en;

    if (!dma_channel_valid(channel) || cb == NULL)
        return false;

    DMA_Init();

    int_en = core_interrupt_disable();

    if (dma_callbacks[channel] != NULL &&
        dma_callbacks[channel] != cb)
    {
        core_restore_interrupt(int_en);
        return false;
    }

    dma_callbacks[channel] = cb;

    core_restore_interrupt(int_en);
    return true;
}
