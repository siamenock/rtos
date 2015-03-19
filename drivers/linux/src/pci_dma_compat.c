#include <asm/pci_dma_compat.h>

void dma_sync_single_for_cpu(PCI_Device *dev, dma_addr_t addr, size_t size, int dir) {
}

void dma_sync_single_for_device(PCI_Device *dev, dma_addr_t addr, size_t size, int dir) {
}

int pci_set_dma_mask(struct pci_dev *dev, u64 mask) {
	return 0;
}

