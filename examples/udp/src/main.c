#include <stdio.h>
#include <thread.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/checksum.h>
#include <net/udp.h>

static uint32_t address = 0xc0a8640a;	// 192.168.100.10

void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
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
		
		if(ip->protocol == IP_PROTOCOL_UDP) {
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
				
				nic_output(nic, packet);
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
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	uint32_t i = 0;
	while(1) {
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
