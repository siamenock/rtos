#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/checksum.h>
#include <net/tcp.h>
#include <readline.h>
#include <pcre.h>

static uint32_t address = 0xc0a8640a;	// 192.168.100.10
static uint64_t gateway;
static pcre* regex = NULL;

void ginit(int argc, char** argv) {
	const char* error;
	int erroffset;
	
        regex = pcre_compile("^([0-9a-zA-Z]{2}):([0-9a-zA-Z]{2}):([0-9a-zA-Z]{2}):([0-9a-zA-Z]{2}):([0-9a-zA-Z]{2}):([0-9a-zA-Z]{2})$", 0, &error, &erroffset, NULL);
        if(!regex) {
                printf("Cannot compile regex\n");
        }
}

void init(int argc, char** argv) {
}

void parse(char* line) {
	uint64_t mac = 0;
	
        int ovector[21];
        int rc = pcre_exec(regex, NULL, line, strlen(line), 0, 0, ovector, 21);
	if(rc == 7) {
		for(int i = 1; i < 7; i++) {
			char* ch = line + ovector[i * 2];
			line[ovector[i * 2 + 1]] = '\0';
			
			mac = (mac << 8) | (strtol(ch, NULL, 16) & 0xff);
		}
	}
	
	printf("Gateway: %02x:%02x:%02x:%02x:%02x:%02x\n", 
		(mac >> 40) & 0xff, (mac >> 32) & 0xff, (mac >> 24) & 0xff,
		(mac >> 16) & 0xff, (mac >> 8) & 0xff, (mac >> 0) & 0xff);
	
	gateway = mac;
}

void process(NIC* nic) {
	Packet* packet = nic_input(nic);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	if(endian16(ether->type) == ETHER_TYPE_ARP) {
		// ARP response
		ARP* arp = (ARP*)ether->payload;
		if(endian16(arp->operation) == 1 && endian32(arp->tpa) == address) {
			ether->dmac = ether->smac;
			ether->smac = endian48(nic->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(address);
			
			nic_output(nic, packet);
			packet = NULL;
		}
	} else if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_TCP) {
			TCP* tcp = (TCP*)ip->body;
			
			if(endian16(tcp->destination) == 7) {
			/*
				uint16_t t = udp->destination;
				udp->destination = udp->source;
				udp->source = t;
				udp->checksum = 0;
				
				uint32_t t2 = ip->destination;
				ip->destination = ip->source;
				ip->source = t2;
				ip->ttl = 0x40;
				ip->checksum = 0;
				ip->checksum = endian16(checksum(ip, ip->ihl * 4));

				uint64_t t3 = ether->dmac;
				ether->dmac = ether->smac;
				ether->smac = t3;
				
				nic_output(nic, packet);
				packet = NULL;
				*/
			}
		}
	}
	
	if(packet)
		nic_free(packet);
}

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	uint32_t i = 0;
	while(1) {
		char* line = readline();
		if(line) {
			parse(line);
		}
		
		uint32_t count = nic_count();
		if(count > 0) {
			i = (i + 1) % count;
			
			NIC* nic = nic_get(i);
			if(nic_has_input(nic)) {
				process(nic);
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
