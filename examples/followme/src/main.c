#include <status.h>
#include <_string.h>
#include <util/types.h>
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
	static uint32_t dip = 0xc0a86465;	// 192.168.100.101 mobile
	static uint64_t dmac = 0xa00bbae1db30;
	
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
			// Packet flow
			if(endian16(udp->destination) == 4321) {
				// Control packet
				uint8_t* body = udp->body;
				dip = body[0] == 0 ? 0xc0a86465 : endian32(ip->source);
				
				switch(dip) {
					case 0xc0a86464:// 192.168.100.100 debian
						dmac = 0x94de806314ab;
						break;
					case 0xc0a86467:// 192.168.100.103 windows
						dmac = 0x681729b8ec7d;
						break;
					default:
						dip = 0xc0a86465;// 192.168.100.101 mobile
						dmac = 0xa00bbae1db30;
				}
				//printf("Forwarding to %x %x\n", dmac, dip);
			} else if(endian16(udp->destination) == 1234) {
				// Forward packet
				udp->checksum = 0;
				
				ip->source = endian32(0xc0a86402);
				ip->destination = endian32(dip);
				ip->ttl--;
				ip->checksum = 0;
				ip->checksum = endian16(checksum(ip, ip->ihl * 4));
				
				ether->dmac = endian48(dmac);
				ether->smac = endian48(ni->mac);
				
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

int main(int argc, char** argv) {
	init(argc, argv);
	
	uint32_t i = 0;
	//while(_app_status == STATUS_START) {
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
	
	destroy();
	
	return 0;
}
