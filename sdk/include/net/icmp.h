#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#define ICMP_LEN		8

#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_ECHO_REQUEST	8

typedef struct {
	uint8_t		type;
	uint8_t		code;
	uint16_t	checksum;
	uint8_t		body[0];
} __attribute__ ((packed)) ICMP;

#endif /* __NET_ICMP_H__ */
