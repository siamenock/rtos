#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>

#include <gmalloc.h>
#include <status.h>
#include <thread.h>
#include <shared.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/nic.h>
#include <net/arp.h>

inline uint64_t cpu_tsc() {
	uint64_t time;
	uint32_t* p = (uint32_t*)&time;
	asm volatile("rdtsc" : "=a"(p[0]), "=d"(p[1]));
	
	return time;
}

void ginit(int argc, char** argv) {
	shared_set(gmalloc(10));
}

void init(int argc, char** argv) {
}

void process(NIC* ni) {
	Packet* packet = nic_input(ni);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	if(endian16(ether->type) == ETHER_TYPE_ARP) {
		// ARP response
		ARP* arp = (ARP*)ether->payload;
		if(endian16(arp->operation) == 1 && endian32(arp->tpa) == 0xc0a86402) {	// 192.168.100.2
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(0xc0a86402);
			
			nic_output(ni, packet);
			packet = NULL;
		}
	} else if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_ICMP && endian32(ip->destination) == 0xc0a86402) {
			// Echo reply
			ICMP* icmp = (ICMP*)ip->body;
			
			icmp->type = 0;
			icmp->checksum = 0;
			icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));
			
			ip->destination = ip->source;
			ip->source = endian32(0xc0a86402);
			ip->ttl = endian8(64);
			ip->checksum = 0;
			ip->checksum = endian16(checksum(ip, ip->ihl * 4));
			
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			
			nic_output(ni, packet);
			packet = NULL;
		} else if(ip->protocol == IP_PROTOCOL_UDP) {
			UDP* udp = (UDP*)ip->body;
			
			if(endian16(udp->destination) == 7) {
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
				
				nic_output(ni, packet);
				packet = NULL;
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
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	
	uint64_t time = cpu_tsc() + 3400000000L * 1;
	while(cpu_tsc() < time);
	/*
	uint32_t i = 0;
	while(1) {
		uint32_t count = nic_count();
		if(count > 0) {
			i = (i + 1) % count;
			
			NIC* ni = nic_get(i);
			if(nic_has_input(ni)) {
				process(ni);
			}
		}
	}
	*/
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
