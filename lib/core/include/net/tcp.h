#ifndef __NET_TCP_H__
#define __NET_TCP_H__

#include <net/ni.h>

#define TCP_LEN			20

typedef struct {
	uint16_t	source;
	uint16_t	destination;
	uint32_t	sequence;
	uint32_t	acknowledgement;
	uint8_t		ns: 1;
	uint8_t		reserved: 3;
	uint8_t		offset: 4;
	uint8_t		fin: 1;
	uint8_t		syn: 1;
	uint8_t		rst: 1;
	uint8_t		psh: 1;
	uint8_t		ack: 1;
	uint8_t		urg: 1;
	uint8_t		ece: 1;
	uint8_t		cwr: 1;
	uint16_t	window;
	uint16_t	checksum;
	uint16_t	urgent;
	
	uint8_t		payload[0];
} __attribute__ ((packed)) TCP;

uint16_t tcp_port_alloc(NetworkInterface* ni);
void tcp_port_free(NetworkInterface* ni, uint16_t port);

#endif /* __NET_TCP_H__ */
