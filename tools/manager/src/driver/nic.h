#ifndef __DRIVER_NIC__
#define __DRIVER_NIC__

#include <stdint.h>
#include <stdbool.h>
#include <util/map.h>
#include "../device.h"

#define MAX_NIC_NAME_LEN	16

typedef struct {
	char		name[MAX_NIC_NAME_LEN];
	uint64_t	mac;

	Map*		vnics;
} NICDevice;

typedef struct {
	char		name[MAX_NIC_NAME_LEN];
	uint64_t	mac;
} NICInfo;

typedef struct {
	int		mtu;
} NICStatus;

typedef struct {
	int		(*init)(void* device, void* data);
	void		(*destroy)(int id);
	int		(*poll)(int id);
	void		(*get_status)(int id, NICStatus* status);
	bool		(*set_status)(int id, NICStatus* status);
	void		(*get_info)(int id, NICInfo* info);
} NICDriver;

void nic_init();
void nic_poll();

NICDevice* nic_create(Device* dev);
void nic_destroy(NICDevice* dev);
NICDevice* nic_device(const char* name);
bool nic_contains(NICDevice* dev, const uint64_t mac);

#endif /* __DRIVER_NIC__ */
