#include <stdio.h>
#include <string.h>
#include <util/map.h>
#include "nic.h"
#include "../dispatcher.h"
#include "../nic.h"
#include "../gmalloc.h"
#include "../device.h"

#define NIC_DEVICE_MAX_COUNT	8

Device* nic_devices[NIC_DEVICE_MAX_COUNT];
Device* nic_current;

void nic_init() {
	uint16_t count = device_count(DEVICE_TYPE_NIC);
	for(int i = 0; i < count; i++) {
		nic_devices[i] = device_get(DEVICE_TYPE_NIC, i);
		if(!nic_devices[i])
			continue;

		if(!nic_create(nic_devices[i]))
			printf("Failed to create NIC device\n");
	}
}

void nic_poll() {
	int poll_count = 0;
	for(int i = 0; i < NIC_MAX_COUNT; i++) {
		Device* dev = nic_devices[i];
		if(dev == NULL)
			break;

		nic_current = dev;
		NICDriver* nic = nic_current->driver;

		poll_count += nic->poll(nic_current->id);
	}
}

NICDevice* nic_create(Device* dev) {
	static int index = 0;
	extern uint64_t manager_mac;
	NICDevice* nic_device = gmalloc(sizeof(NICDevice));
	if(!nic_device)
		return NULL;

	nic_device->vnics = map_create(8, NULL, NULL, gmalloc_pool);
	dev->priv = nic_device;

	NICInfo info;
	info.name[0] = '\0';
	((NICDriver*)dev->driver)->get_info(dev->id, &info);

	nic_device->mac = info.mac;
	if(info.name[0] != '\0') {
		strncpy(nic_device->name, info.name, sizeof(nic_device->name));
	} else {
		sprintf(nic_device->name, "eth%d", index);
		index++;
	}

	nic_device->vnics = map_create(16, NULL, NULL, gmalloc_pool);

	int rc = dispatcher_create_nic((void*)nic_device);
	if(rc < 0) {
		printf("Failed to create NIC in dispatcher module\n");
		return NULL;
	}

	printf("\tNICs in physical NIC(%s): %p\n", nic_device->name, nic_device->vnics);
	printf("\t%s : [%02lx:%02lx:%02lx:%02lx:%02lx:%02lx] [%c]\n", nic_device->name,
			(info.mac >> 40) & 0xff,
			(info.mac >> 32) & 0xff,
			(info.mac >> 24) & 0xff,
			(info.mac >> 16) & 0xff,
			(info.mac >> 8) & 0xff,
			(info.mac >> 0) & 0xff,
			manager_mac == 0 ? '*' : ' ');

	if(!manager_mac)
		manager_mac = info.mac;

	index++;

	return nic_device;
}

void nic_destroy(NICDevice* dev) {
}

NICDevice* nic_device(const char* name) {
	for(int i = 0; i < NIC_MAX_COUNT; i++) {
		Device* dev = nic_devices[i];
		if(dev == NULL)
			break;

		NICDevice* nic = dev->priv;
		if(!strncmp(nic->name, name, MAX_NIC_NAME_LEN))
			return nic;
	}

	return NULL;
}

bool nic_contains(NICDevice* dev, const uint64_t mac) {
	Map* vnics = dev->vnics;
	if(!vnics)
		return false;

	return map_contains(vnics, (void*)mac);
}
