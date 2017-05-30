#ifndef __NET_INTERFACE_H__
#define __NET_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <util/set.h>
#include <nic.h>

typedef struct _IPv4Interface {
	uint32_t	address;
	uint32_t	gateway;
	uint32_t	netmask;
} IPv4Interface;

#define IPV4_INTERFACE_MAX_COUNT	16

typedef struct _IPv4InterfaceTable {
	uint16_t		bitmap;
	uint8_t			default_idx;
	IPv4Interface		interfaces[IPV4_INTERFACE_MAX_COUNT];
} IPv4InterfaceTable;

typedef struct _IPv6Interface {
	uint32_t	address[4];
	uint32_t	gateway[4];
	uint32_t	netmask[4];
	bool		_default;	// Move to config
} IPv6Interface;

IPv4InterfaceTable* interface_table_get(NIC* nic);

IPv4Interface* interface_alloc(NIC* nic, uint32_t address, uint32_t netmask, uint32_t gateway, bool is_default);
bool interface_free(NIC* nic, uint32_t address);

IPv4Interface* interface_get_default(NIC* nic);
IPv4Interface* interface_get(NIC* nic, uint32_t address);

#endif /* __NET_INTERFACE_H__ */
