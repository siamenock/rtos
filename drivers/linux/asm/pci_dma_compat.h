#ifndef __ASM_GENERIC_PCI_DMA_COMPAT_H__
#define __ASM_GENERIC_PCI_DMA_COMPAT_H__

#include <linux/types.h>
#include <linux/pci.h>

void dma_sync_single_for_cpu(PCI_Device *dev, dma_addr_t addr, size_t size, int dir);
void dma_sync_single_for_device(PCI_Device *dev, dma_addr_t addr, size_t size, int dir);
int pci_set_dma_mask(PCI_Device *dev, u64 mask);

#endif /* __ASM_GENERIC_PCI_DMA_COMPAT_H__ */

