#ifndef __DRIVER_DEVICE__
#define __DRIVER_DEVICE__

typedef enum {
	DEVICE_TYPE_NULL,
	DEVICE_TYPE_CHAR_IN,
	DEVICE_TYPE_CHAR_OUT,
	DEVICE_TYPE_NIC,
	DEVICE_TYPE_FILESYSTEM,
	DEVICE_TYPE_PORT,
} DeviceType;

DeviceType device_type;

typedef struct {
	int(*init)(void* device, void* data);
	void(*destroy)(int id);
} Driver;

#endif /* __DRIVER_DEVICE__ */
