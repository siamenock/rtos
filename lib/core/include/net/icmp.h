#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#include <stdbool.h>
#include <net/packet.h>

/**
 * @file
 * Internet Control Message Protocol version 4
 */

#define ICMP_LEN		8	///< ICMPv4 header length

#define ICMP_TYPE_ECHO_REPLY	0	///< ICMPv4 type, echo reply
#define ICMP_TYPE_ECHO_REQUEST	8	///< ICMPv4 type, echo request

/**
 * ICMPv4 header
 */
typedef struct _ICMP {
	uint8_t		type;		///< Type
	uint8_t		code;		///< Sub-type
	uint16_t	checksum;	///< ICMP header and data checksum (endian16)
	uint8_t		body[0];	///< ICMP body
} __attribute__ ((packed)) ICMP;

/**
 * Process ICMPv4 packet
 *
 * @param ICMPv4 packet
 * @return true if the packet is processed, in the case the packet must not be reused
 */
bool icmp_process(Packet* packet);

#endif /* __NET_ICMP_H__ */
