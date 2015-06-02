#include <time.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/checksum.h>
#include <util/map.h>

#define TCP_PORTS	".pn.tcp.ports"
#define TCP_NEXT_PORT	".pn.tcp.next_port"

uint16_t tcp_port_alloc(NetworkInterface* ni) {
	Map* ports = ni_config_get(ni, TCP_PORTS);
	if(!ports) {
		ports = map_create(64, map_uint64_hash, map_uint64_equals, ni->pool);
		ni_config_put(ni, TCP_PORTS, ports);
	}
	
	uint16_t port = (uint16_t)(uint64_t)ni_config_get(ni, TCP_NEXT_PORT);
	if(port < 49152)
		port = 49152;
	
	while(map_contains(ports, (void*)(uint64_t)port)) {
		if(++port < 49152)
			port = 49152;
	}	
	
	map_put(ports, (void*)(uint64_t)port, (void*)(uint64_t)port);
	ni_config_put(ni, TCP_NEXT_PORT, (void*)(uint64_t)(port + 1));
	
	return port;
}

void tcp_port_free(NetworkInterface* ni, uint16_t port) {
	Map* ports = ni_config_get(ni, TCP_PORTS);
	if(!ports)
		return;
	
	map_remove(ports, (void*)(uint64_t)port);
}

void tcp_pack(Packet* packet, uint16_t tcp_body_len) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	TCP* tcp = (TCP*)ip->body;
	
	uint16_t tcp_len = TCP_LEN + tcp_body_len;
	
	TCP_Pseudo pseudo;
	pseudo.source = ip->source;
	pseudo.destination = ip->destination;
	pseudo.padding = 0;
	pseudo.protocol = ip->protocol;
	pseudo.length = endian16(tcp_len);
	
	tcp->checksum = 0;
	uint32_t sum = (uint16_t)~checksum(&pseudo, sizeof(pseudo)) + (uint16_t)~checksum(tcp, tcp_len);
	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	tcp->checksum = endian16(~sum);
	
	ip_pack(packet, tcp_len);
}
