#ifndef __LINUX_DMA_MAPPING_H__
#define __LINUX_DMA_MAPPING_H__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/dma-direction.h>

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ull : ((1ull << (n)) - 1))
#define DMA_MASK_NONE   0x0ULL

#define DEFINE_DMA_UNMAP_ADDR(ADDR_NAME)        	dma_addr_t ADDR_NAME
#define DEFINE_DMA_UNMAP_LEN(LEN_NAME)        		__u32 LEN_NAME
#define dma_map_single(dev, cpu_addr, size, dir)	(cpu_addr)
#define dma_map_page(dev, page, offset, size, dir)	(page + offset)
#define dma_unmap_addr(PTR, ADDR_NAME)  	        ((PTR)->ADDR_NAME)
#define dma_unmap_addr_set(PTR, ADDR_NAME, VAL)  (((PTR)->ADDR_NAME) = (VAL))
#define dma_unmap_len(PTR, LEN_NAME)			((PTR)->LEN_NAME)
#define dma_unmap_len_set(PTR, LEN_NAME, VAL)  		(((PTR)->LEN_NAME) = (VAL))
#define dma_mapping_error(dev, addr)	(false)

int dma_set_mask(struct device *dev, u64 mask);
int dma_set_coherent_mask(struct device *dev, u64 mask);
int dma_set_mask_and_coherent(struct device *dev, u64 mask);
void* dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp);
void dma_free_coherent(struct device *dev, size_t size, void *kvaddr, dma_addr_t dma_handle);

static inline void dma_unmap_page(struct device *dev, dma_addr_t dma_handle, size_t size, enum dma_data_direction dir) {}

static inline void dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size, enum dma_data_direction dir) {}

#endif /* __LINUX_DMA-MAPPING_H__ */

