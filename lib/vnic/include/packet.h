#ifndef __PACKET_H__
#define __PACKET_H__

#ifndef MODULE
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#else
// Linux kernel modules such as dispatcher cannot include glibc header files
#include <linux/types.h>
#endif

/**
 * @file
 * Packet data structure
 */

/**
 * Packet data structure
 */
typedef struct _Packet {
	uint64_t	time;	    ///< epoch timestamp

	uint16_t	vlan_proto; ///< VLAN Protocol
	uint16_t	vlan_tci;   ///< VLAN TCI

	// Actual packet data size: end-start
	uint16_t	start;	    ///< start offset
	uint16_t	end;	    ///< end offset

	uint16_t	size;	    ///< size of allocated buffer
	uint8_t		buffer[0];  ///< data buffer
} Packet;

#endif /*__PACKET_H__*/
