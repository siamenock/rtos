#include <stdio.h>
#include <stdlib.h>
#include <util/event.h>
#include <timer.h>
#include "apic.h"
#include "icc.h"
#include "mapping.h"
#include "shared.h"
#include "gmalloc.h"
#include "vm.h"
#include "manager.h"
#include "shell.h"
#include "malloc.h"
#include "vnic.h"
#include "socket.h"
#include "driver/nic.h"

static bool idle0_event() {
	extern Device* nic_current;
	int poll_count = 0;
	for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		Device* dev = nic_devices[i];
		if(dev == NULL)
			break;

		nic_current = dev;
		NICDriver* nic = nic_current->driver;

		poll_count += nic->poll(nic_current->id);
	}
	return true;
}

static void init_nics(int count) {
	int index = 0;
	extern uint64_t manager_mac;
	for(int i = 0; i < count; i++) {
		nic_devices[i] = device_get(DEVICE_TYPE_NIC, i);
		if(!nic_devices[i])
			continue;

		NICPriv* nic_priv = gmalloc(sizeof(NICPriv));
		if(!nic_priv)
			continue;

		nic_priv->nics = map_create(8, NULL, NULL, NULL);

		nic_devices[i]->priv = nic_priv;

		NICInfo info;
		((NICDriver*)nic_devices[i]->driver)->get_info(nic_devices[i]->id, &info);

		nic_priv->port_count = info.port_count;
		for(int j = 0; j < info.port_count; j++) {
			nic_priv->mac[j] = info.mac[j];

			char name_buf[64];
			sprintf(name_buf, "eth%d", index);
			uint16_t port = j << 12;

			Map* vnics = map_create(16, NULL, NULL, NULL);
			map_put(nic_priv->nics, (void*)(uint64_t)port, vnics);

			printf("\t%s : [%02lx:%02lx:%02lx:%02lx:%02lx:%02lx] [%c]\n", name_buf,
					(info.mac[j] >> 40) & 0xff,
					(info.mac[j] >> 32) & 0xff,
					(info.mac[j] >> 24) & 0xff,
					(info.mac[j] >> 16) & 0xff,
					(info.mac[j] >> 8) & 0xff,
					(info.mac[j] >> 0) & 0xff,
					manager_mac == 0 ? '*' : ' ');

			if(!manager_mac)
				manager_mac = info.mac[j];

			index++;
		}
	}
}

int main() {
	printf("\nInitailizing local APIC...\n");
	if(apic_init() < 0)
		goto error;

	printf("\nInitializing memory mapping...\n");
	if(mapping_init() < 0)
		goto error;

	printf("\nInitializing shared memory area...\n");
	shared_init();

	printf("\nInitializing mulitiprocessor core data...\n");
	mp_init();

	printf("\nInitializing gmalloc area...\n");
	gmalloc_init();

	printf("\nInitializing local malloc area...\n");
	malloc_init();

	printf("\nInitializing events...\n");
	event_init();

	printf("\nInitializing inter-core communications...\n");
	icc_init();

	printf("Initializing linux socket device...\n");
	socket_init();

	uint16_t nic_count = device_count(DEVICE_TYPE_NIC);
	printf("Initializing NICs: %d\n", nic_count);
	init_nics(nic_count);

	printf("Initializing VM manager...\n");
	vm_init();

	printf("Initializing RPC manager...\n");
	manager_init();

	printf("Initializing shell...\n");
	shell_init();

	event_idle_add(idle0_event, NULL);

	/*
	 *apic_write64(APIC_REG_ICR, ((uint64_t)1 << 56) |
	 *                APIC_DSH_NONE |
	 *                APIC_TM_EDGE |
	 *                APIC_LV_DEASSERT |
	 *                APIC_DM_PHYSICAL |
	 *                APIC_DMODE_FIXED |
	 *                48);
	 */

	while(1)
		event_loop();

	return 0;
error:
	printf("Initial memory mapping error occured. Terminated...\n");
	return -1;
}
