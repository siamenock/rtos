#ifndef __DRIVER_NIC__
#define __DRIVER_NIC__

#include <stdint.h>
#include <stdbool.h>
#include "../device.h"

typedef struct {
	uint64_t	mac;
} NICStatus;

typedef struct {
	int	(*init)(void* device, void* data);
	void	(*destroy)(int id);
	int	(*poll)(int id);
	void	(*get_status)(int id, NICStatus* status);
	bool	(*set_status)(int id, NICStatus* status);
} NIC;

#endif /* __DRIVER_NIC__ */
