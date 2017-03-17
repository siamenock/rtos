#ifndef __NET_UDP_H__
#define __NET_UDP_H__

#include <net/packet.h>
#include <net/nic.h>

/**
 * @file
 * User Datagram Protocol verion 4
 */

#define UDP_LEN			8		///< UDPv4 header length

/**
 * UDPv4 header
 */
typedef struct _UDP {
	uint16_t	source;			///< Source port number (endian16)
	uint16_t	destination;		///< Destination port number (endian16)
	uint16_t	length;			///< Header and data length
	uint16_t	checksum;		///< Header and data checksum
	
	uint8_t		body[0];
} __attribute__ ((packed)) UDP;

/**
 * Allocate UDP port number which associated with NI.
 *
 * @param nic NIC reference
 * @param addr
 * @param port
 * @return true if port alloc success
 */
bool udp_port_alloc0(NIC* nic, uint32_t addr, uint16_t port);

/**
 * Allocate UDP port number which associated with NI.
 *
 * @param nic NIC reference
 * @param addr
 * @return port number
 */
uint16_t udp_port_alloc(NIC* nic, uint32_t addr);

/**
 * Free UDP port number.
 *
 * @param nic NIC reference
 * @param addr
 * @param port port number to free
 */
void udp_port_free(NIC* nic, uint32_t addr, uint16_t port);

/**
 * Set UDP length, checksum, and do IP packing.
 *
 * @param packet UDP packet to pack
 * @param udp_body_len UDP body length in bytes
 */
void udp_pack(Packet* packet, uint16_t udp_body_len);

#endif /* __NET_UDP_H__ */
