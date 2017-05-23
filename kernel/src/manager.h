#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>
#include <util/list.h>
#include "vnic.h"

#define DEFAULT_MANAGER_IP	0xc0a864fe	// 192.168.100.254
#define DEFAULT_MANAGER_GW	0xc0a86401	// 192.168.100.1
#define DEFAULT_MANAGER_NETMASK	0xffffff00	// 255.255.255.0
#define DEFAULT_MANAGER_PORT	1111

typedef struct _Manager {
	int		nic_count;
	VNIC*		nics[NIC_MAX_COUNT];	// gmalloc, ni_create
	List* netifs;
} Manager;

extern Manager manager;

int manager_init();
uint32_t manager_get_ip();
void manager_set_ip(uint32_t ip);
uint16_t manager_get_port();
void manager_set_port(uint16_t port);
uint32_t manager_get_gateway();
void manager_set_gateway(uint32_t gw);
uint32_t manager_get_netmask();
void manager_set_netmask(uint32_t nm);
void manager_set_interface();

#endif /* __MANAGER_H__ */
