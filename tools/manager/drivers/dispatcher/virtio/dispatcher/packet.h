#ifndef __NET_PACKET__
#define __NET_PACKET__

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
	
	unsigned int	status;		///< Packet status (deprecated)
	unsigned long	time;		///< Packet in time in CPU clock
	
	unsigned short	start;		///< Packet payload starting index of payload buffer
	unsigned short	end;		///< Packet payload ending index of payload buffer
	unsigned short	size;		///< Packet buffer size
	
	unsigned char		buffer[0];	///< Packet buffer which contains packet payload itself
} Packet;

/**
 * Dump packet payload to standard output
 * Don't dare use this function in production, but use it for debugging only.
 * @param packet packet reference
 */
void packet_dump(Packet* packet);

#endif /* __NET_PACKET__ */
