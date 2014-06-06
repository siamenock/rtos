#ifndef __NET_UDP_H__
#define __NET_UDP_H__

#define UDP_LEN			8

typedef struct {
	uint16_t	source;
	uint16_t	destination;
	uint16_t	length;
	uint16_t	checksum;
	
	uint8_t		body[0];
} __attribute__ ((packed)) UDP;

#endif /* __NET_UDP_H__ */
