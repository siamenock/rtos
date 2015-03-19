#ifndef __LINUX_DMA_DIRECTION_H__
#define __LINUX_DMA_DIRECTION_H__

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2, // 0 in linux.h
	DMA_NONE = 3,
};

#endif /* __LINUX_DMA_DIRECTION_H__ */

