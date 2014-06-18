#ifndef __NET_ARP_H__
#define __NET_ARP_H__

#include <stdint.h>
#include <stdbool.h>
#include <net/packet.h>
#include <net/ni.h>

#define ARP_LEN		28

typedef struct {
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t operation;
	uint64_t sha: 48;
	uint32_t spa;
	uint64_t tha: 48;
	uint32_t tpa;
} __attribute__ ((packed)) ARP;

extern uint32_t ARP_TIMEOUT;

bool arp_process(Packet* packet);

uint64_t arp_get_mac(NetworkInterface* ni, uint32_t ip);
uint32_t arp_get_ip(NetworkInterface* ni, uint64_t mac);

#endif /* __NET_ARP_H__ */
