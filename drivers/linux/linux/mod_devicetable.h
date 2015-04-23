#ifndef __LINUX_MOD_DEVICETABLE_H__
#define __LINUX_MOD_DEVICETABLE_H__

#include <linux/types.h>

#define PCI_ANY_ID (~0)

#define VIRTIO_DEV_ANY_ID	0xffffffff
struct pci_device_id {
	uint32_t	vendor;
	uint32_t	device;
	uint32_t	subvendor;
	uint32_t	subdevice;
	uint32_t	class;
	uint32_t	class_mask;
	kernel_ulong_t	driver_data;
};

struct virtio_device_id { 
	__u32 device;
	__u32 vendor;
};

#endif /* __LINUX_MOD_DEVICETABLE_H__ */

