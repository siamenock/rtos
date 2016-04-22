#include <stdio.h>
#include <string.h>
#include <status.h>
#include <malloc.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/nic.h>
#include <net/arp.h>

void init(int argc, char** argv) {
}

void process(NIC* ni) {
	Packet* packet = nic_input(ni);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	//printf("process %lx %lx %x, size=%d\n", endian48(ether->dmac), endian48(ether->smac), endian16(ether->type), packet->end - packet->start);
	
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
	while(__app_status == APP_STATUS_STARTED) {
		uint32_t count = nic_count();
		if(count > 0) {
			i = (i + 1) % count;
			
			NIC* ni = nic_get(i);
			if(nic_has_input(ni)) {
				process(ni);
			}
		}
	}
	
	destroy();
	
	return 0;
}
