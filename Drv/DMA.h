#ifndef VENDOR_BRACELET_DRV_DMA_H_
#define VENDOR_BRACELET_DRV_DMA_H_

#include "tl_common.h"

typedef void (*DMA_Callback)(dma_chn_e channel);

/*
 * TL721X has one PLIC source for all DMA channels.
 * This module owns that single IRQ and dispatches terminal-count
 * notifications to per-channel callbacks.
 */
void DMA_Init(void);
bool DMA_RegisterCallback(dma_chn_e channel, DMA_Callback cb);

#endif /* VENDOR_BRACELET_DRV_DMA_H_ */
