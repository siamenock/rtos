#ifndef __LINUX_PCI_H__
#define __LINUX_PCI_H__

#include <packetngin/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/dma-direction.h>
#include <linux/time.h>

#define PCI_DEVICE(_vendor, _device)	\
	.vendor = (_vendor),	\
	.device = (_device),	\
	.subvendor = PCI_ANY_ID,\
	.subdevice = PCI_ANY_ID

void* pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t *dma_handle);
void pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle);

int pci_enable_device(struct pci_dev *dev);
void pci_disable_device(struct pci_dev *dev);

void pci_set_master(struct pci_dev *dev);
void pci_clear_master(struct pci_dev *dev);

#define pci_enable_msi(pdev)    pci_enable_msi_exact(pdev, 1)

int pci_enable_msi_range(struct pci_dev *dev, int minvec, int maxvec);
void pci_disable_msi(struct pci_dev *dev);

inline int pci_enable_msi_exact(struct pci_dev *dev, int nvec) {
	int rc = pci_enable_msi_range(dev, nvec, nvec);
	if (rc < 0)
		return rc;
	return 0;
}

dma_addr_t pci_resource_start(struct pci_dev *dev, int region);
dma_addr_t pci_resource_end(struct pci_dev *dev, int region);
dma_addr_t pci_resource_flags(struct pci_dev *dev, int region);
#define pci_resource_len(dev,bar) \
	((pci_resource_start((dev), (bar)) == 0 && pci_resource_end((dev), (bar)) == pci_resource_start((dev), (bar))) ? \
	0 : (pci_resource_end((dev), (bar)) - pci_resource_start((dev), (bar)) + 1))

void *pci_get_drvdata(struct pci_dev *pdev);
void pci_set_drvdata(struct pci_dev *pdev, void *data);

typedef unsigned int pci_channel_state_t;

enum pci_channel_state {
	/* I/O channel is in normal state */
	pci_channel_io_normal = (pci_channel_state_t) 1,
	/* I/O to channel is blocked */
	pci_channel_io_frozen = (pci_channel_state_t) 2,
	/* PCI card is dead */
	pci_channel_io_perm_failure = (pci_channel_state_t) 3,
};

typedef unsigned int pci_ers_result_t;
enum pci_ers_result {
	/* no result/none/not supported in device driver */
	PCI_ERS_RESULT_NONE = (pci_ers_result_t) 1,
	/* Device driver can recover without slot reset */
	PCI_ERS_RESULT_CAN_RECOVER = (pci_ers_result_t) 2,
	/* Device driver wants slot to be reset. */
	PCI_ERS_RESULT_NEED_RESET = (pci_ers_result_t) 3,
	/* Device has completely failed, is unrecoverable */
	PCI_ERS_RESULT_DISCONNECT = (pci_ers_result_t) 4,
	/* Device driver is fully recovered and operational */
	PCI_ERS_RESULT_RECOVERED = (pci_ers_result_t) 5,
	/* No AER capabilities registered for the driver */
	PCI_ERS_RESULT_NO_AER_DRIVER = (pci_ers_result_t) 6,
};

struct pci_error_handlers {
	/* PCI bus error detected on this device */
	pci_ers_result_t (*error_detected)(struct pci_dev *dev, enum pci_channel_state error);
	/* MMIO has been re-enabled, but not DMA */
	pci_ers_result_t (*mmio_enabled)(struct pci_dev *dev);
	/* PCI Express link has been reset */
	pci_ers_result_t (*link_reset)(struct pci_dev *dev);
	/* PCI slot has been reset */
	pci_ers_result_t (*slot_reset)(struct pci_dev *dev);
	/* PCI function reset prepare or completed */
	void (*reset_notify)(struct pci_dev *dev, bool prepare);
	/* Device driver may resume normal operations */
	void (*resume)(struct pci_dev *dev);
};

struct pci_driver {
	const char* name;
	const struct pci_device_id *id_table;
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);
	void (*remove) (struct pci_dev *dev);
	const struct pci_error_handlers *err_handler;
};

int pci_register_driver(struct pci_driver *);
void pci_unregister_driver(struct pci_driver *dev);

#define PCI_DMA_BIDIRECTIONAL	0
#define PCI_DMA_TODEVICE	1
#define PCI_DMA_FROMDEVICE	2
#define PCI_DMA_NONE		3

dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr, size_t size, int direction);
void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr, size_t size, int direction);

int pci_dma_mapping_error(struct pci_dev *pdev, dma_addr_t dma_addr);

#endif /* __LINUX_PCI_H__ */

