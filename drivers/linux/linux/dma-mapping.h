#ifndef __LINUX_DMA_MAPPING_H__
#define __LINUX_DMA_MAPPING_H__

#include <linux/device.h>

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ull : ((1ull << (n)) - 1))
#define DMA_MASK_NONE   0x0ULL

int dma_set_mask(struct device *dev, u64 mask);
int dma_set_coherent_mask(struct device *dev, u64 mask);

#endif /* __LINUX_DMA-MAPPING_H__ */

