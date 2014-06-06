#ifndef __DRIVER_PCI__
#define __DRIVER_PCI__

#include <stdint.h>

#define PCI_ID_ANY	~(0)

typedef struct _PCI_ID {
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint16_t	subvendor_id;
	uint16_t	subdevice_id;
	char*		name;
	void*		data;
} PCI_ID;

#define PCI_DEVICE(_vendor_id, _device_id, _name, _data) {	\
	.vendor_id = _vendor_id, .device_id = _device_id, 	\
	.subvendor_id = PCI_ID_ANY, .subdevice_id = PCI_ID_ANY, \
	.name = _name, .data = (void*)_data }

#define PCI_DEVICE2(_vendor_id, _device_id, _subvendor_id, _subdevice_id, _name, _data) {\
	.vendor_id = _vendor_id, .device_id = _device_id, 				\
	.subvendor_id = _subvendor_id, .subdevice_id = _subdevice_id,			\
	.name = _name, .data = (void*)_data }


#endif /* __DRIVER_PCI__ */
