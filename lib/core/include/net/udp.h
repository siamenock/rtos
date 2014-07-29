#ifndef __NET_UDP_H__
#define __NET_UDP_H__

#include <net/packet.h>
#include <net/ni.h>

#define UDP_LEN			8

typedef struct {
	uint16_t	source;
	uint16_t	destination;
	uint16_t	length;
	uint16_t	checksum;
	
	uint8_t		body[0];
} __attribute__ ((packed)) UDP;

void udp_pack(Packet* packet, uint16_t udp_body_len);

uint16_t udp_port_alloc(NetworkInterface* ni);
void udp_port_free(NetworkInterface* ni, uint16_t port);

#endif /* __NET_UDP_H__ */
