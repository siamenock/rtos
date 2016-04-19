#ifndef __NET_TCP_H__
#define __NET_TCP_H__

#include <net/nic.h>

/**
 * @file
 * Transmission Control Protocol verion 4
 */

#define TCP_LEN			20		///< TCPv4 header length

/**
 * TCPv4 header
 */
typedef struct _TCP {
	uint16_t	source;			///< Source port number (endian16)
	uint16_t	destination;		///< Destination port number (endian16)
	uint32_t	sequence;		///< Sequence number (endian32)
	uint32_t	acknowledgement;	///< Acknowledgement number (endian32)
	uint8_t		ns: 1;			///< ECN-nonce concealment protection flag
	uint8_t		reserved: 3;		///< Reserved
	uint8_t		offset: 4;		///< Data offset, TCP header length in 32-bit words
	uint8_t		fin: 1;			///< No more data from sender flag
	uint8_t		syn: 1;			///< Synchronize sequence number flag
	uint8_t		rst: 1;			///< Reset the connection flag
	uint8_t		psh: 1;			///< Push flag
	uint8_t		ack: 1;			///< Acknowledgement field is significant flag
	uint8_t		urg: 1;			///< Urgent pointer is significant flag
	uint8_t		ece: 1;			///< ECN-Echo flag
	uint8_t		cwr: 1;			///< Congestion Window Reduced flag
	uint16_t	window;			///< Window size (endian16)
	uint16_t	checksum;		///< Header and data checksum (endian16)
	uint16_t	urgent;			///< Urgent pointer (endian16)
	
	uint8_t		payload[0];		///< TCP payload
} __attribute__ ((packed)) TCP;

/**
 * TCPv4 pseudo header to calculate header checksum
 */
typedef struct _TCP_Pseudo {
	uint32_t        source;			///< Source address (endian32)
	uint32_t        destination;		///< Destination address (endian32)
	uint8_t         padding;		///< Zero padding
	uint8_t         protocol;		///< TCP protocol number, 0x06
	uint16_t        length;			///< Header and data length in bytes (endian32)
} __attribute__((packed)) TCP_Pseudo;

/**
 * Allocate TCP port number which associated with NI.
 *
 * @param nic NIC reference
 * @param addr
 * @param port
 * @return true if port alloc success
 */
bool tcp_port_alloc0(NIC* nic, uint32_t addr, uint16_t port);

/**
 * Allocate TCP port number which associated with NI.
 *
 * @param nic NIC reference
 * @param addr
 * @return port number
 */
uint16_t tcp_port_alloc(NIC* nic, uint32_t addr);

/**
 * Free TCP port number.
 *
 * @param nic NIC reference
 * @param addr
 * @param port port number to free
 */
void tcp_port_free(NIC* nic, uint32_t addr, uint16_t port);

/**
 * Set TCP checksum, and do IP packing.
 *
 * @param packet TCP packet to pack
 * @param tcp_body_len TCP body length in bytes
 */
void tcp_pack(Packet* packet, uint16_t tcp_body_len);

#endif /* __NET_TCP_H__ */
