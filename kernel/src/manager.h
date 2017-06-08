#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>
#include <util/list.h>
#include <vnic.h>

#define MANAGER_DEFAULT_NICDEV	"eth0"
#define MANAGER_DEFAULT_IP	0xc0a864fe	// 192.168.100.254
#define MANAGER_DEFAULT_GW	0xc0a86401	// 192.168.100.1
#define MANAGER_DEFAULT_NETMASK	0xffffff00	// 255.255.255.0
#define MANAGER_DEFAULT_PORT	1111

typedef struct _Manager {
	int		vnic_count;
	VNIC*		vnics[NIC_MAX_COUNT];	// gmalloc, ni_create
	List* 		netifs;
	List*		servers;
	List* 		clients;
	List* 		actives;
} Manager;

extern Manager manager;

int manager_init();

#endif /* __MANAGER_H__ */
