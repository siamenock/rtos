#include <linux/dma-mapping.h>
#include <gmalloc.h>

// arch/x86/kernel/pci-dma.c
int dma_set_mask(struct device *dev, u64 mask) { 
	return 0; 
}

int dma_set_coherent_mask(struct device *dev, u64 mask) {
	return 0;
}

int dma_set_mask_and_coherent(struct device *dev, u64 mask) {
	return 0;
}

#define PAGE_SHIFT 12
void* dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp) {
	uint64_t align;

	if(size <= 0)
		return NULL;

	/* Check the size is whether to be square of 2 or not */
	if((size & (size - 1)) != 0)
		align = 1 << PAGE_SHIFT; // default is 4kb alignment 	
	else
		align = size;

	void *ptr;
	void *p = gmalloc(size + align - 1 + sizeof(void*));

	if (p != NULL) {
		/* Address of the aligned memory according to the align parameter*/
		ptr = (void*)(((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
		/* store the address of the malloc() above
		 * at the beginning of our total memory area.
		 * You can also use *((void **)ptr-1) = p
		 * instead of the one below.
		 */
		*((void**)((uint64_t)ptr - sizeof(void*))) = p;
		/* Return the address of aligned memory */
		*dma_handle = (dma_addr_t)ptr;
		return ptr; 
	}
	return NULL;
}

void dma_free_coherent(struct device *dev, size_t size, void *kvaddr, dma_addr_t dma_handle) {
	void* ptr = *((void**)((uint64_t)kvaddr - sizeof(void*)));

	if(ptr)
		gfree(ptr);
}
