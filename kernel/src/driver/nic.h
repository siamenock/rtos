#ifndef __DRIVER_NIC__
#define __DRIVER_NIC__

#include <stdint.h>
#include <stdbool.h>
#include <util/map.h>
#include "../device.h"

#define MAX_PORT_COUNT	16

typedef struct {
	uint8_t		port_count;
	uint64_t	mac[MAX_PORT_COUNT];
	/***
	 * nics
	 *     key: port(4bits) | vlan_id(12bits)
	 *     value: Map* -> VNICs
	 * VNICS
	 *     key: mac
	 *     value: VNIC
	 */
	Map*		nics;
} NICPriv;

typedef struct {
	uint8_t		port_count;
	uint64_t	mac[MAX_PORT_COUNT];
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
} NICDriver;

#endif /* __DRIVER_NIC__ */
