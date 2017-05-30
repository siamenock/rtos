#include <nic.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/interface.h>
#include <util/map.h>

bool icmp_process(NIC* nic, Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_IPv4)
		return false;
	
	IP* ip = (IP*)ether->payload;
	uint32_t address = endian32(ip->destination);

	if(!interface_get(nic, address))
		return false;
	
	if(ip->protocol == IP_PROTOCOL_ICMP) {
		ICMP* icmp = (ICMP*)((uint8_t*)ip + ip->ihl * 4);

		switch(icmp->type) {
			case ICMP_TYPE_ECHO_REQUEST:
				icmp->type = 0;
				icmp->checksum = 0;
				icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));

				swap32(ip->source, ip->destination);
				ip->ttl = endian8(64);
				ip->checksum = 0;
				ip->checksum = endian16(checksum(ip, ip->ihl * 4));

				swap48(ether->smac, ether->dmac);

				nic_tx(nic, packet);

				return true;
			// TODO
		}
	}
	
	return false;
}

bool icmp_request(NIC* nic, uint32_t address) {
	//TODO
	return true;
}
