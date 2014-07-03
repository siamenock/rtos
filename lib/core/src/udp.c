#include <malloc.h>
#include <time.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/udp.h>
#include <util/map.h>

static Map* ports;

void udp_pack(Packet* packet, uint16_t udp_body_len) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	UDP* udp = (UDP*)ip->body;
	
	uint16_t udp_len = UDP_LEN + udp_body_len;
	udp->length = endian16(udp_len);
	udp->checksum = 0;	// TODO: Calc checksum
	packet->end = ((void*)udp + udp_len) - (void*)packet->buffer;
	
	ip_pack(packet, udp_len);
}

uint16_t udp_port_alloc() {
	if(!ports) {
		ports = map_create(64, map_uint64_hash, map_uint64_equals, malloc, free);
	}
	
	uint16_t port = (uint16_t)clock();
	while(port < 49152 || map_contains(ports, (void*)(uint64_t)port)) {
		port = (uint16_t)clock();
	}
	
	map_put(ports, (void*)(uint64_t)port, (void*)(uint64_t)port);
	
	return port;
}

void udp_port_free(uint16_t port) {
	if(!ports)
		return;
	
	map_remove(ports, (void*)(uint64_t)port);
}
