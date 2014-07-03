#ifndef __NET_ETHER_H__
#define __NET_ETHER_H__

#include <stdint.h>
#include <byteswap.h>

#define ETHER_MULTICAST	((uint64_t)1 << 40)

#define ETHER_TYPE_IPv4		0x0800
#define ETHER_TYPE_ARP		0x0806
#define ETHER_TYPE_IPv6		0x86dd
#define ETHER_TYPE_LLDP		0x88cc
#define ETHER_TYPE_8021Q	0x8100
#define ETHER_TYPE_QINQ1	0x9100
#define ETHER_TYPE_QINQ2	0x9200
#define ETHER_TYPE_QINQ3	0x9300

#define ETHER_LEN	14

typedef struct {
	uint64_t dmac: 48;
	uint64_t smac: 48;
	uint16_t type;
	uint8_t payload[0];
} __attribute__ ((packed)) Ether;

#define endian8(v)	(v)
#define endian16(v)	bswap_16((v))
#define endian32(v)	bswap_32((v))
#define endian48(v)	(bswap_64((v)) >> 16)
#define endian64(v)	bswap_64((v))

uint8_t read_u8(void* buf, uint32_t* idx);
uint16_t read_u16(void* buf, uint32_t* idx);
uint32_t read_u32(void* buf, uint32_t* idx);
uint64_t read_u48(void* buf, uint32_t* idx);
uint64_t read_u64(void* buf, uint32_t* idx);
char* read_string(void* buf, uint32_t* idx);	// Read C string without '\0'
void write_u8(void* buf, uint8_t value, uint32_t* idx);
void write_u16(void* buf, uint16_t value, uint32_t* idx);
void write_u32(void* buf, uint32_t value, uint32_t* idx);
void write_u48(void* buf, uint64_t value, uint32_t* idx);
void write_u64(void* buf, uint64_t value, uint32_t* idx);
void write_string(void* buf, const char* str, uint32_t* idx);	// Write C string without '\0'

#define swap8(v1, v2) do {	\
	uint8_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);

#define swap16(v1, v2) do {	\
	uint16_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);

#define swap32(v1, v2) do {	\
	uint32_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);

#define swap48(v1, v2) do {	\
	uint64_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);

#define swap64(v1, v2) do {	\
	uint64_t tmp = (v1);	\
	(v1) = (v2);		\
	(v2) = tmp;		\
} while(0);

#endif /* __NET_ETHER_H__ */
