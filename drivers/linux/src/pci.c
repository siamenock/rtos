#include <linux/pci.h>
#include <gmalloc.h>

#define PAGE_SHIFT 12

void* pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t *dma_handle) { 
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
		ptr = (void*) (((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
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

void pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle) {
	void* ptr = *((void**)((uint64_t)vaddr - sizeof(void*)));

	if(ptr)
		gfree(ptr);
}

int printf(const char *format, ...);

bool pci_enable(PCI_Device* device) {
	bool changed = false;
	
	uint16_t enable = PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
	uint16_t command = pci_read16(device, PCI_COMMAND);
	if((command & enable) != enable) {
		command |= enable;
		if(command & PCI_COMMAND_INTX_DISABLE) {
			printf("Interrupt Disabled\n");
		}
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
	return -1;
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
//	printf("mask %d, io %p mem %p, ori : %p\n", addr & PCI_BASE_ADDRESS_SPACE_IO, addr & PCI_BASE_ADDRESS_IO_MASK, addr & PCI_BASE_ADDRESS_MEM_MASK, addr);
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

volatile void __iomem *pci_iomap(struct pci_dev *dev, int bar, unsigned long maxlen)
{
	resource_size_t start = pci_resource_start(dev, bar);

	return (void*)start;
//	resource_size_t len = pci_resource_len(dev, bar);
//	unsigned long flags = pci_resource_flags(dev, bar);
//
//	if (!len || !start)
//		return NULL;
//	if (maxlen && len > maxlen)
//		len = maxlen;
//	if (flags & IORESOURCE_IO)
//		return __pci_ioport_map(dev, start, len);
//	if (flags & IORESOURCE_MEM) {
//		if (flags & IORESOURCE_CACHEABLE)
//			return ioremap(start, len);
//		return ioremap_nocache(start, len);
//	}
//	/* What? */
//	return NULL;
}

void pci_iounmap(struct pci_dev *dev, volatile void __iomem * addr)
{}

//void pci_msi_off(struct pci_dev *dev)
//{
//	int pos;
//	u16 control;
//
//	/*
//	 * This looks like it could go in msi.c, but we need it even when
//	 * CONFIG_PCI_MSI=n.  For the same reason, we can't use
//	 * dev->msi_cap or dev->msix_cap here.
//	 */
//	pos = pci_find_capability(dev, PCI_CAP_ID_MSI);
//	if (pos) {
//		pci_read_config_word(dev, pos + PCI_MSI_FLAGS, &control);
//		control &= ~PCI_MSI_FLAGS_ENABLE;
//		pci_write_config_word(dev, pos + PCI_MSI_FLAGS, control);
//	}
//	pos = pci_find_capability(dev, PCI_CAP_ID_MSIX);
//	if (pos) {
//		pci_read_config_word(dev, pos + PCI_MSIX_FLAGS, &control);
//		control &= ~PCI_MSIX_FLAGS_ENABLE;
//		pci_write_config_word(dev, pos + PCI_MSIX_FLAGS, control);
//	}
//}
//
//int pci_find_capability(struct pci_dev *dev, int cap)
//{
//	int pos;
//
//	pos = __pci_bus_find_cap_start(dev->bus, dev->devfn, dev->hdr_type);
//	if (pos)
//		pos = __pci_find_next_cap(dev->bus, dev->devfn, pos, cap);
//
//	return pos;
//}
//
//static int __pci_bus_find_cap_start(struct pci_bus *bus,
//				    unsigned int devfn, u8 hdr_type)
//{
//	u16 status;
//
//	pci_bus_read_config_word(bus, devfn, PCI_STATUS, &status);
//	if (!(status & PCI_STATUS_CAP_LIST))
//		return 0;
//
//	switch (hdr_type) {
//	case PCI_HEADER_TYPE_NORMAL:
//	case PCI_HEADER_TYPE_BRIDGE:
//		return PCI_CAPABILITY_LIST;
//	case PCI_HEADER_TYPE_CARDBUS:
//		return PCI_CB_CAPABILITY_LIST;
//	default:
//		return 0;
//	}
//
//	return 0;
//}
//
//#define PCI_FIND_CAP_TTL	48
//
//static int __pci_find_next_cap_ttl(struct pci_bus *bus, unsigned int devfn,
//				   u8 pos, int cap, int *ttl)
//{
//	u8 id;
//
//	while ((*ttl)--) {
//		pci_bus_read_config_byte(bus, devfn, pos, &pos);
//		if (pos < 0x40)
//			break;
//		pos &= ~3;
//		pci_bus_read_config_byte(bus, devfn, pos + PCI_CAP_LIST_ID,
//					 &id);
//		if (id == 0xff)
//			break;
//		if (id == cap)
//			return pos;
//		pos += PCI_CAP_LIST_NEXT;
//	}
//	return 0;
//}
//
//static int __pci_find_next_cap(struct pci_bus *bus, unsigned int devfn,
//			       u8 pos, int cap)
//{
//	int ttl = PCI_FIND_CAP_TTL;
//
//	return __pci_find_next_cap_ttl(bus, devfn, pos, cap, &ttl);
//}
