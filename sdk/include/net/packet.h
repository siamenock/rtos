#ifndef __NET_PACKET__
#define __NET_PACKET__

#include <stdint.h>

typedef struct _NetworkInterface NetworkInterface;

typedef struct _Packet {
	NetworkInterface*	ni;
	
	uint32_t		status;
	uint64_t		time;
	
	uint16_t		start;
	uint16_t		end;
	uint16_t		size;
	
	uint8_t			buffer[0];
} Packet;

void packet_dump(Packet* packet);

#endif /* __NET_PACKET__ */
