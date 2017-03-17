#ifndef __NET_IP_H__
#define __NET_IP_H__

#include <net/packet.h>

/**
 * @file
 * Internet Protocol version 4
 */

#define IP_LEN			20	///< IPv4 header length
#define IPDEFTTL		64	///< Default IPv4 TTL value

#define IP_PROTOCOL_ICMP	0x01	///< IP protocol number for ICMP
#define IP_PROTOCOL_IP          0x04	///< IP protocol number for IP
#define IP_PROTOCOL_UDP		0x11	///< IP protocol number for UDP
#define IP_PROTOCOL_TCP		0x06	///< IP protocol number for TCP
#define IP_PROTOCOL_ESP         0x32	///< IP protocol number for ESP
#define IP_PROTOCOL_AH          0x33	///< IP protocol number for AH

/**
 * Set IP flags and offset at once
 */
#define ip_flags_offset(flag, offset) endian16((((uint16_t)(flag) & 0x07) << 13) | ((uint16_t)(offset) & 0x1fff))

/**
 * IPv4 header
 */
typedef struct _IP {
	uint8_t		ihl: 4;		///< Internet Header Length in number of 32-bit words in the header
	uint8_t		version: 4;	///< IP version, it must be 4
	uint8_t		ecn: 2;		///< Explicit Congestion Notification
	uint8_t		dscp: 6;	///< Differentiated Services Code Point
	uint16_t	length;		///< Header and body length in bytes (endian16)
	uint16_t	id;		///< Identification (endian16)
	uint16_t 	flags_offset;	///< Flags(3 bits): More fragments bit, Don't fragment bit, 0(reserved). Offset: Fragment offset (endian16)
	uint8_t		ttl;		///< Time to live
	uint8_t		protocol;	///< Protocol number
	uint16_t	checksum;	///< Header checksum (endian16)
	uint32_t	source;		///< Source IP address (endian32)
	uint32_t	destination;	///< Destination IP address (endian32)
	
	uint8_t		body[0];	///< IP body payload
} __attribute__ ((packed)) IP;

/**
 * Set IP length, TTL, checksum, and Packet->end index
 *
 * @param packet packet reference
 * @param ip_body_len IP body length in bytes
 */
void ip_pack(Packet* packet, uint16_t ip_body_len);
 
#endif /* __NET_IP_H__ */
