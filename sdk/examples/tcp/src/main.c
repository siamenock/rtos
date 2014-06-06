#include <stdio.h>
#include <thread.h>
#include <net/ni.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/tcp.h>

void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

static uint32_t address = 0xc0a8640a;	// 192.168.100.10

void process(NetworkInterface* ni) {
	Packet* packet = ni_input(ni);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	if(endian16(ether->type) == ETHER_TYPE_ARP) {
		// ARP response
		ARP* arp = (ARP*)ether->payload;
		if(endian16(arp->operation) == 1 && endian32(arp->tpa) == address) {
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(address);
			
			ni_output(ni, packet);
			packet = NULL;
		}
	} else if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_ICMP && endian32(ip->destination) == address) {
			// Echo reply
			ICMP* icmp = (ICMP*)ip->body;
			
			icmp->type = 0;
			icmp->checksum = 0;
			icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));
			
			ip->destination = ip->source;
			ip->source = endian32(address);
			ip->ttl = endian8(64);
			ip->checksum = 0;
			ip->checksum = endian16(checksum(ip, ip->ihl * 4));
			
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			
			ni_output(ni, packet);
			packet = NULL;
		} else if(ip->protocol == IP_PROTOCOL_TCP) {
			TCP* tcp = (TCP*)ip->body;
			
			TCP_Pseudo_Header header;
			header.source = endian32(address);
			header.destination = 
			
			TCP_Option* option = tcp_option(tcp);
			while(option) {
				switch(option->kind) {
					case TCP_OPTION_EOL:
						printf("EOL\n");
						break;
					case TCP_OPTION_NOP:
						printf("NOP\n");
						break;
					case TCP_OPTION_MSS:
						printf("MSS: %d\n", endian16(option->data.u16));
						break;
					case TCP_OPTION_WSOPT:
						printf("WSOPT: %d\n", endian8(option->data.u8));
						break;
					case TCP_OPTION_SACKP:
						printf("SACKP: true\n");
						break;
					case TCP_OPTION_TSOPT:
						{
							printf("TSOPT: ");
							for(int i = 0; i < option->length - 2; i++)
								printf("%02x", option->data.payload[i]);
							printf("\n");
						}
						break;
					default:
						printf("Unknown option: %d\n", option->kind);
				}
				
				option = tcp_option_next(tcp, option);
			}
		}
	}
	
	if(packet)
		ni_free(packet);
}

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
	printf("Thread %d bootting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	uint32_t i = 0;
	while(1) {
		uint32_t count = ni_count();
		if(count > 0) {
			i = (i + 1) % count;
			
			NetworkInterface* ni = ni_get(i);
			if(ni_has_input(ni)) {
				process(ni);
			}
		}
	}
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
