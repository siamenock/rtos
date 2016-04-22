#include <string.h>
#include <status.h>
#include <util/types.h>//#include "pn/types.h"
#include <net/ether.h>//#include "pn/ether.h"
#include <net/ip.h>//#include "pn/ip.h"
#include <net/icmp.h>//#include "pn/icmp.h"
#include <net/checksum.h>//#include "pn/checksum.h"
#include <net/udp.h>//#include "pn/udp.h"
#include <net/nic.h>//#include "pn/ni.h"
#include <net/arp.h>

typedef struct {
	char		collection[256];
	uint64_t	id;
} ID;

static ID ids[10];

static uint16_t seq;
static int reply_count;
static uint64_t user_mac;
static uint32_t user_ip;
static uint16_t user_port;;

static uint64_t newID(char* collection) {
	int i;
	for(i = 0; i < 9; i++) {
		if(ids[i].collection) {
			if(strcmp(ids[i].collection, collection) == 0)
				break;
		} else {
			int clen = strlen(collection) + 1;
			memcpy(ids[i].collection, collection, clen);
			break;
		}
	}
	
	return ids[i].id++;
}

void init(int argc, char** argv) {
}

void process(NIC* ni) {
	static uint32_t myip = 0xc0a86402;	// 192.168.100.2
	
	Packet* packet = nic_input(ni);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) == ETHER_TYPE_ARP) {
		// ARP response
		ARP* arp = (ARP*)ether->payload;
		if(endian16(arp->operation) == 1 && endian32(arp->tpa) == myip) {
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(myip);
			
			nic_output(ni, packet);
			packet = NULL;
		}
	} else if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_ICMP && endian32(ip->destination) == myip) {
			// Echo reply
			ICMP* icmp = (ICMP*)ip->body;
			
			icmp->type = 0;
			icmp->checksum = 0;
			icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));
			
			ip->destination = ip->source;
			ip->source = endian32(myip);
			ip->ttl = endian8(64);
			ip->checksum = 0;
			ip->checksum = endian16(checksum(ip, ip->ihl * 4));
			
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			
			nic_output(ni, packet);
			packet = NULL;
		} else if(ip->protocol == IP_PROTOCOL_UDP) {
			UDP* udp = (UDP*)ip->body;
			
			if(endian16(udp->destination) == 9000) {
				reply_count = 2;
				
				// Control packet
				uint32_t idx = 0;
				seq = read_u16(udp->body, &idx);
				user_mac = endian48(ether->smac);
				user_ip = endian32(ip->source);
				user_port = endian16(udp->source);
				uint8_t msg = read_u8(udp->body, &idx);
				switch(msg) {
					case 1: // MSG_CREATE
					{
						idx++;	// read ctype
						uint32_t clen = read_u32(udp->body, &idx);
						char collection[clen + 1];
						collection[clen] = '\0';
						memcpy(collection, udp->body + idx, clen);
						idx += clen;
						
						uint64_t id = newID(collection);
						memmove(udp->body + idx + 9, udp->body + idx, endian16(udp->length) - 8 - idx);
						packet->end += 9;
						
						udp->length = endian16(endian16(udp->length) + 9);
						ip->length = endian16(endian16(ip->length) + 9);
						
						write_u8(udp->body, 4, &idx);
						write_u64(udp->body, id, &idx);
					}
						break;
					case 2: // MSG_READ
						reply_count = 1;
						break;
					case 3: // MSG_RETRIEVE
						break;
					case 4: // MSG_UPDATE
						break;
					case 5: // MSG_DELETE
						break;
					case 6: // MSG_HELLO
						reply_count = 1;
						break;
				}
				
				udp->source = endian16(9999);
				udp->destination = endian16(9001);
				udp->checksum = 0;
				
				ip->destination = ip->source;
				ip->source = endian32(myip);
				ip->ttl = endian8(ip->ttl) - 1;
				ip->checksum = 0;
				ip->checksum = endian16(checksum(ip, ip->ihl * 4));
				
				ether->dmac = ether->smac;
				ether->smac = endian48(ni->mac);
				
				nic_output_dup(ni, packet);
				
				udp->destination = endian16(9002);
				ip->checksum = 0;
				ip->checksum = endian16(checksum(ip, ip->ihl * 4));
				nic_output_dup(ni, packet);
				
				udp->destination = endian16(9003);
				ip->checksum = 0;
				ip->checksum = endian16(checksum(ip, ip->ihl * 4));
				nic_output(ni, packet);
				packet = NULL;
			} else if(endian16(udp->destination) == 9999) {
				uint32_t idx = 0;
				uint16_t seq2 = read_u16(udp->body, &idx);
				if(seq == seq2 && --reply_count == 0) {
					udp->checksum = 0;
					udp->destination = endian16(user_port);
					udp->source = endian16(9000);
					
					ip->destination = endian32(user_ip);
					ip->source = endian32(myip);
					ip->ttl = endian8(64);
					ip->checksum = 0;
					ip->checksum = endian16(checksum(ip, ip->ihl * 4));
					
					ether->dmac = endian48(user_mac);
					ether->smac = endian48(ni->mac);
					
					nic_output(ni, packet);
					packet = NULL;
				}
			}
		}
	}
	
	if(packet)
		nic_free(packet);
}

void destroy() {
}

int main(int argc, char** argv) {
	init(argc, argv);
	
	uint32_t i = 0;
//	while(_app_status == APP_STATUS_STARTED) {
	while(1){
		uint32_t count = nic_count();
		if(count > 0) {
			i = (i + 1) % count;
			
			NIC* ni = nic_get(i);
			if(nic_has_input(ni)) {
				process(ni);
			}
		}
	}
//	}
	
	destroy();
	
	return 0;
}
