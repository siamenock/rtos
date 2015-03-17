#ifndef __LINUX_MOD_DEVICETABLE_H__
#define __LINUX_MOD_DEVICETABLE_H__

#include <linux/types.h>

struct pci_device_id {
	uint32_t	vendor;
	uint32_t	device;
	uint32_t	subvendor;
	uint32_t	subdevice;
	uint32_t	class;
	uint32_t	class_mask;
	kernel_ulong_t	driver_data;
};

#endif /* __LINUX_MOD_DEVICETABLE_H__ */

