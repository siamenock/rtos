#include <linux/pci.h>
#include <gmalloc.h>

void* pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t *dma_handle) { 
	void* memory = gmalloc(size * 2);
	memory = (void*)(((uint64_t)memory + (size - 1)) & ~(size - 1ul));
	*dma_handle = (dma_addr_t)memory;
	
	return (void*)*dma_handle;
}

void pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle) {
	// TODO: need to free memory before alignment 
	if(vaddr)
		gfree(vaddr);
}

bool pci_enable(PCI_Device* device) {
	bool changed = false;
	
	uint16_t enable = PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
	uint16_t command = pci_read16(device, PCI_COMMAND);
	if((command & enable) != enable) {
		command |= enable;
		pci_write16(device, PCI_COMMAND, command);
		changed = true;
	}
	
	uint8_t latency_timer = pci_read8(device, PCI_LATENCY_TIMER);
	if(latency_timer < 32) {
		pci_write8(device, PCI_LATENCY_TIMER, 32);
		changed = true;
	}
	
	return changed;
}

int pci_enable_device(struct pci_dev *dev) {
	pci_enable(dev);
	return 0;
}
void pci_disable_device(struct pci_dev *dev) {}

void pci_set_master(struct pci_dev *dev) {
	// Already done when enabled
}

void pci_clear_master(struct pci_dev *dev) {}

int pci_enable_msi_range(struct pci_dev *dev, int minvec, int maxvec) {
	// PacketNgin doesn't have msi
	return 0;
}
void pci_disable_msi(struct pci_dev *dev) {
	// PacketNgin doesn't have msi
}

void pci_disable_link_state(struct pci_dev *dev, int state) {
	// TODO: Implement it
}

int pci_enable_msi_exact(struct pci_dev *dev, int nvec) {
	int rc = pci_enable_msi_range(dev, nvec, nvec);
	if (rc < 0)
		return rc;
	return 0;
}

dma_addr_t pci_resource_start(struct pci_dev *dev, int region) {
	uint32_t addr = pci_read32(dev, PCI_BASE_ADDRESS_0 + region * 4);
	if(addr & PCI_BASE_ADDRESS_SPACE_IO)
		return addr & PCI_BASE_ADDRESS_IO_MASK;
	else
		return addr & PCI_BASE_ADDRESS_MEM_MASK;
}

dma_addr_t pci_resource_end(struct pci_dev *dev, int region) {
	return 0;
}

dma_addr_t pci_resource_flags(struct pci_dev *dev, int region) {	
	return 0;
}

void *pci_get_drvdata(struct pci_dev *pdev) {
	return pdev->driver;
}

void pci_set_drvdata(struct pci_dev *pdev, void *data) {
	pdev->driver = data;
}

int pci_write_config_byte(struct pci_dev *dev, int where, u8 val) {
	pci_write8(dev, where, val);
	return 0;
}

int pci_write_config_word(struct pci_dev *dev, int where, u16 val) {
	pci_write16(dev, where, val);
	return 0;
}

int pci_read_config_byte(struct pci_dev *dev, int where, u8* val) {
	*val = pci_read8(dev, where);
	return 0;
}

int pci_read_config_word(struct pci_dev *dev, int where, u16* val) {
        *val = pci_read16(dev, where);
	return 0;
}

int pcie_capability_clear_word(struct pci_dev *dev, int pos, u16 clear) {
	u16 val;

	val = pci_read16(dev, pos);
	val &= ~clear;
	pci_write16(dev, pos, val);

	return 0;
}

int pcie_capability_set_word(struct pci_dev *dev, int pos, u16 set) {
	u16 val;

	val = pci_read16(dev, pos);
	val |= set;
	pci_write16(dev, pos, val);

	return 0;
}

int pcie_capability_clear_and_set_word(struct pci_dev *dev, int pos, u16 clear, u16 set) {
	u16 val;

	val = pci_read16(dev, pos);
	val &= ~clear;
	val |= set;
	pci_write16(dev, pos, val);

	return 0;
}

void pci_release_regions(struct pci_dev *pdev) {
	// TODO: Implement it
}

int pci_set_cacheline_size(struct pci_dev *dev) {
	return 0;
}

int pci_set_mwi(struct pci_dev *dev) {
	// TODO: Check cache line size
	uint16_t cmd = pci_read16(dev, PCI_COMMAND);
	if(!(cmd & PCI_COMMAND_INVALIDATE)) {
		cmd |= PCI_COMMAND_INVALIDATE;
		pci_write16(dev, PCI_COMMAND, cmd);
	}

	return 0;
}

void pci_clear_mwi(struct pci_dev *dev) {
}

int pci_is_pcie(struct pci_dev *dev) {
	return dev->caps[PCI_CAP_ID_EXP] != 0;
}

int pci_register_driver(struct pci_driver *drv) {
	/* initialize common driver fields */
//	drv->driver.name = drv->name;
//	drv->driver.bus = &pci_bus_type;
//	drv->driver.owner = owner;
//	drv->driver.mod_name = mod_name;
//
//	spin_lock_init(&drv->dynids.lock);
//	INIT_LIST_HEAD(&drv->dynids.list);

	/* register with core */
//	return driver_register(&drv->driver);
	return 0;
}

void pci_unregister_driver(struct pci_driver *dev) {}

dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr, size_t size, int direction) { 
	return (dma_addr_t)ptr;
}

void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr, size_t size, int direction) {}

int pci_dma_mapping_error(struct pci_dev *pdev, dma_addr_t dma_addr) { 
	return 0; 
}
