#ifndef VENDOR_BRACELET_BSP_DMA_H_
#define VENDOR_BRACELET_BSP_DMA_H_

#include "tl_common.h"

/* TL721X DMA channels are numbered from DMA0. */
#define BSP_DMA_MAX_CHANNELS 8

typedef void (*BSP_DMA_Callback)(dma_chn_e channel);

void BSP_DMA_Init(void);
bool BSP_DMA_RegisterCallback(dma_chn_e channel, BSP_DMA_Callback callback);

#endif /* VENDOR_BRACELET_BSP_DMA_H_ */
