#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <linux/types.h>

typedef enum {
	DEVICE_TYPE_NULL,
	DEVICE_TYPE_CHAR_IN,
	DEVICE_TYPE_CHAR_OUT,
	DEVICE_TYPE_NIC,
	DEVICE_TYPE_FILESYSTEM,
	DEVICE_TYPE_PORT,
	DEVICE_TYPE_VIRTIO_BLK,
} DeviceType;

typedef struct device {
	DeviceType	type;
	void*		driver;	// Extends Driver
	void*		device;
	int		id;
	void*		priv;
} Device;

#endif /* __DEVICE_H__ */
