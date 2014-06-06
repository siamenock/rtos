#ifndef __DRIVER_DEVICE__
#define __DRIVER_DEVICE__

typedef enum {
	DEVICE_TYPE_NULL,
	DEVICE_TYPE_CHAR_IN,
	DEVICE_TYPE_CHAR_OUT,
	DEVICE_TYPE_NIC,
} DeviceType;

DeviceType device_type;

#endif /* __DRIVER_DEVICE__ */
