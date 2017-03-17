#ifndef __NET_PACKET__
#define __NET_PACKET__

#include <stdint.h>

/**
 * @file
 * Packet structure
 */

typedef struct _NIC NIC;

/**
 * A Packet.
 */
typedef struct _Packet {
	NIC*		nic;		///< Associated NIC which allocated the Packet
	
	uint32_t	status;		///< Packet status (deprecated)
	uint64_t	time;		///< Packet in time in CPU clock
	
	uint16_t	start;		///< Packet payload starting index of payload buffer
	uint16_t	end;		///< Packet payload ending index of payload buffer
	uint16_t	size;		///< Packet buffer size
	
	uint8_t		buffer[0];	///< Packet buffer which contains packet payload itself
} Packet;

/**
 * Dump packet payload to standard output
 * Don't dare use this function in production, but use it for debugging only.
 * @param packet packet reference
 */
void packet_dump(Packet* packet);

#endif /* __NET_PACKET__ */
