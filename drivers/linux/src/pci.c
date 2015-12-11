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

void pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle) {
	void* ptr = *((void**)((uint64_t)vaddr - sizeof(void*)));

	if(ptr)
		gfree(ptr);
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

static inline u16 pcie_caps_reg(struct pci_dev *dev) {
	return pci_read16(dev, dev->caps[PCI_CAP_ID_EXP] + PCI_EXP_FLAGS);
}

static inline int pci_pcie_type(struct pci_dev *dev)
{
	return (pcie_caps_reg(dev) & PCI_EXP_FLAGS_TYPE) >> 4;
}

bool pcie_downstream_port(struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return type == PCI_EXP_TYPE_ROOT_PORT ||
	       type == PCI_EXP_TYPE_DOWNSTREAM;
}

static inline bool pcie_cap_has_lnkctl(struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return type == PCI_EXP_TYPE_ENDPOINT ||
	       type == PCI_EXP_TYPE_LEG_END ||
	       type == PCI_EXP_TYPE_ROOT_PORT ||
	       type == PCI_EXP_TYPE_UPSTREAM ||
	       type == PCI_EXP_TYPE_DOWNSTREAM ||
	       type == PCI_EXP_TYPE_PCI_BRIDGE ||
	       type == PCI_EXP_TYPE_PCIE_BRIDGE;
}

static inline bool pcie_cap_has_sltctl(struct pci_dev *dev)
{
	return pcie_downstream_port(dev) &&
	       pcie_caps_reg(dev) & PCI_EXP_FLAGS_SLOT;
}

bool pcie_capability_reg_implemented(struct pci_dev *dev, int pos)
{
	if (!pci_is_pcie(dev))
		return false;

	switch (pos) {
	case PCI_EXP_FLAGS:
		return true;
	case PCI_EXP_DEVCAP:
	case PCI_EXP_DEVCTL:
	case PCI_EXP_DEVSTA:
		return true;
	case PCI_EXP_LNKCAP:
	case PCI_EXP_LNKCTL:
	case PCI_EXP_LNKSTA:
		return pcie_cap_has_lnkctl(dev);
	/* Start of GurumNetworks modification
	case PCI_EXP_SLTCAP:
	case PCI_EXP_SLTCTL:
	case PCI_EXP_SLTSTA:
		return pcie_cap_has_sltctl(dev);
	case PCI_EXP_RTCTL:
	case PCI_EXP_RTCAP:
	case PCI_EXP_RTSTA:
		return pcie_cap_has_rtctl(dev);
	case PCI_EXP_DEVCAP2:
	case PCI_EXP_DEVCTL2:
	case PCI_EXP_LNKCAP2:
	case PCI_EXP_LNKCTL2:
	case PCI_EXP_LNKSTA2:
		return pcie_cap_version(dev) > 1;
	End of GurumNetworks modificatoin */
	default:
		return false;
	}
}

void pci_dump(struct pci_dev *dev) {
	int idx = 0;
	while(idx < 256) {
		printf("%8x", pci_read32(dev, idx));
		idx += 4;
	}
}
