#include <net/ni.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <util/map.h>

bool icmp_process(Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_IPv4)
		return false;
	
	uint32_t addr = (uint32_t)(uint64_t)ni_config_get(packet->ni, "ip");
	if(!addr)
		return false;
	
	IP* ip = (IP*)ether->payload;
	if(ip->protocol == IP_PROTOCOL_ICMP && endian32(ip->destination) == addr) {
		ICMP* icmp = (ICMP*)ip->body;
		
		icmp->type = 0;
		icmp->checksum = 0;
		icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));
		
		swap32(ip->source, ip->destination);
		ip->ttl = endian8(64);
		ip->checksum = 0;
		ip->checksum = endian16(checksum(ip, ip->ihl * 4));
		
		swap48(ether->smac, ether->dmac);
		
		ni_output(packet->ni, packet);
		
		return true;
	}
	
	return false;
}
