#include <linux/dma-mapping.h>

// arch/x86/kernel/pci-dma.c
int dma_set_mask(struct device *dev, u64 mask)
{ 
	return 0; 
}

int dma_set_coherent_mask(struct device *dev, u64 mask) 
{
	return 0;
}

