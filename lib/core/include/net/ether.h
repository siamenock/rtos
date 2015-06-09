#ifndef __NET_ETHER_H__
#define __NET_ETHER_H__

#include <stdint.h>
#include <byteswap.h>

/**
 * @file
 * Ethernet header and basic macros to manage packets
 */

#define ETHER_MULTICAST	((uint64_t)1 << 40)	///< MAC address is multicast

#define ETHER_TYPE_IPv4		0x0800		///< Ether type of IPv4
#define ETHER_TYPE_ARP		0x0806		///< Ether type of ARP
#define ETHER_TYPE_IPv6		0x86dd		///< Ether type of IPv6
#define ETHER_TYPE_LLDP		0x88cc		///< Ether type of LLDP
#define ETHER_TYPE_8021Q	0x8100		///< Ether type of 802.1q
#define ETHER_TYPE_QINQ1	0x9100		///< Ether type of QinQ
#define ETHER_TYPE_QINQ2	0x9200		///< Ether type of QinQ
#define ETHER_TYPE_QINQ3	0x9300		///< Ether type of QinQ

#define ETHER_LEN		14		///< Ehternet header length

/**
 * Ethernet header
 */
typedef struct _Ether {
	uint64_t dmac: 48;			///< Destination address (endian48)
	uint64_t smac: 48;			///< Destination address (endian48)
	uint16_t type;				///< Ether type (endian16)
	uint8_t payload[0];			///< Ehternet payload
} __attribute__ ((packed)) Ether;

#define endian8(v)	(v)			///< Change endianness for 8 bits
#define endian16(v)	bswap_16((v))		///< Change endianness for 16 bits
#define endian32(v)	bswap_32((v))		///< Change endianness for 32 bits
#define endian48(v)	(bswap_64((v)) >> 16)	///< Change endianness for 48 bits
#define endian64(v)	bswap_64((v))		///< Change endianness for 64 bits

/**
 * Read unsigned 8 bits integer from buffer and change endianness.
 *
 * @param buf buffer to read from
 * @param[in,out] idx the index to read from, the index will be changed after read
 * @return interger value
 */
uint8_t read_u8(void* buf, uint32_t* idx);

/**
 * Read unsigned 16 bits integer from buffer and change endianness.
 *
 * @param buf buffer to read from
 * @param[in,out] idx the index to read from, the index will be changed after read
 * @return interger value
 */
uint16_t read_u16(void* buf, uint32_t* idx);

/**
 * Read unsigned 32 bits integer from buffer and change endianness.
 *
 * @param buf buffer to read from
 * @param[in,out] idx the index to read from, the index will be changed after read
 * @return interger value
 */
uint32_t read_u32(void* buf, uint32_t* idx);

/**
 * Read unsigned 48 bits integer from buffer and change endianness.
 *
 * @param buf buffer to read from
 * @param[in,out] idx the index to read from, the index will be changed after read
 * @return interger value
 */
uint64_t read_u48(void* buf, uint32_t* idx);

/**
 * Read unsigned 64 bits integer from buffer and change endianness.
 *
 * @param buf buffer to read from
 * @param[in,out] idx the index to read from, the index will be changed after read
 * @return interger value
 */
uint64_t read_u64(void* buf, uint32_t* idx);

/**
 * Read C style string from buffer without '\0'.
 *
 * @param buf buffer to read from
 * @param[in,out] idx the index to read from, the index will be changed after read
 * @return buf + *idx
 */
char* read_string(void* buf, uint32_t* idx);

/**
 * Change endianness and write unsigned 8 bits integer to buffer.
 *
 * @param buf buffer to write to
 * @param value integer value
 * @param[in,out] idx the index to write to, the index will be changed after written
 */
void write_u8(void* buf, uint8_t value, uint32_t* idx);

/**
 * Change endianness and write unsigned 16 bits integer to buffer.
 *
 * @param buf buffer to write to
 * @param value integer value
 * @param[in,out] idx the index to write to, the index will be changed after written
 */
void write_u16(void* buf, uint16_t value, uint32_t* idx);

/**
 * Change endianness and write unsigned 32 bits integer to buffer.
 *
 * @param buf buffer to write to
 * @param value integer value
 * @param[in,out] idx the index to write to, the index will be changed after written
 */
void write_u32(void* buf, uint32_t value, uint32_t* idx);

/**
 * Change endianness and write unsigned 48 bits integer to buffer.
 *
 * @param buf buffer to write to
 * @param value integer value
 * @param[in,out] idx the index to write to, the index will be changed after written
 */
void write_u48(void* buf, uint64_t value, uint32_t* idx);

/**
 * Change endianness and write unsigned 64 bits integer to buffer.
 *
 * @param buf buffer to write to
 * @param value integer value
 * @param[in,out] idx the index to write to, the index will be changed after written
 */
void write_u64(void* buf, uint64_t value, uint32_t* idx);

/**
 * Write C style string to buffer without '\0'.
 *
 * @param buf buffer to write to
 * @param str the string to write
 * @param[in,out] idx the index to write to, the index will be changed after written
 */
void write_string(void* buf, const char* str, uint32_t* idx);	// Write C string without '\0'

#define swap8(v1, v2) do {	\
	uint8_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);				///< Swap unsigned 8 bits values

#define swap16(v1, v2) do {	\
	uint16_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);				///< Swap unsigned 16 bits values

#define swap32(v1, v2) do {	\
	uint32_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);				///< Swap unsigned 32 bits values

#define swap48(v1, v2) do {	\
	uint64_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);				///< Swap unsigned 48 bits values

#define swap64(v1, v2) do {	\
	uint64_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);				///< Swap unsigned 64 bits values

#endif /* __NET_ETHER_H__ */
