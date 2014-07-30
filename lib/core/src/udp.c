#include <time.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/udp.h>
#include <util/map.h>

#define UDP_PORTS	".pn.udp.ports"

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

uint16_t udp_port_alloc(NetworkInterface* ni) {
	Map* ports = ni_config_get(ni, UDP_PORTS);
	if(!ports) {
		ports = map_create(64, map_uint64_hash, map_uint64_equals, ni->pool);
		ni_config_put(ni, UDP_PORTS, ports);
	}
	
	uint16_t port = (uint16_t)clock();
	while(port < 49152 || map_contains(ports, (void*)(uint64_t)port)) {
		port = (uint16_t)clock();
	}
	
	map_put(ports, (void*)(uint64_t)port, (void*)(uint64_t)port);
	
	return port;
}

void udp_port_free(NetworkInterface* ni, uint16_t port) {
	Map* ports = ni_config_get(ni, UDP_PORTS);
	if(!ports)
		return;
	
	map_remove(ports, (void*)(uint64_t)port);
}
