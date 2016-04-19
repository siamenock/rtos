#ifndef __NET_INTERFACE_H__
#define __NET_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <util/set.h>

#define NIC_ADDR_IPv4	"net.addr.ipv4"
#define NIC_ADDR_IPv6	"net.addr.ipv6"

typedef struct _IPv4Interface {
	uint32_t	gateway;
	uint32_t	netmask;
	bool		_default;	// Move to config
	
	Set*		udp_ports;
	uint16_t	udp_next_port;
	Set*		tcp_ports;
	uint16_t	tcp_next_port;
} IPv4Interface;

typedef struct _IPv6Interface {
	uint32_t	gateway[4];
	uint32_t	netmask[4];
	bool		_default;	// Move to config
	
	Set*		udp_ports;
	uint16_t	udp_next_port;
	Set*		tcp_ports;
	uint16_t	tcp_next_port;
} IPv6Interface;

IPv4Interface* interface_alloc(void* pool);
void interface_free(IPv4Interface* interface, void* pool);

#endif /* __NET_INTERFACE_H__ */
