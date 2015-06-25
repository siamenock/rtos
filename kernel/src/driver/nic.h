#ifndef __DRIVER_NIC__
#define __DRIVER_NIC__

#include <stdint.h>
#include <stdbool.h>
#include "../device.h"

typedef struct {
	uint8_t		port_count;
	uint64_t*	mac;
} NICInfo;

typedef struct {
} NICStatus;

typedef struct {
	int	(*init)(void* device, void* data);
	void	(*destroy)(int id);
	int	(*poll)(int id);
	void	(*get_status)(int id, NICStatus* status);
	bool	(*set_status)(int id, NICStatus* status);
	void	(*get_info)(int id, NICInfo* info);
} NIC;

#endif /* __DRIVER_NIC__ */
