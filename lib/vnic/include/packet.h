#ifndef __PACKET_H__
#define __PACKET_H__

#ifndef MODULE
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#else
#include <linux/types.h>
#endif

typedef struct _Packet {
	uint64_t	time;

	uint16_t	start;
	uint16_t	end;
	uint16_t	size;

	uint8_t		buffer[0];
} Packet;

#endif /*__PACKET_H__*/
