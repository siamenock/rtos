#include <malloc.h>
#include <time.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <util/map.h>

static Map* ports;

uint16_t tcp_port_alloc() {
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

void tcp_port_free(uint16_t port) {
	if(!ports)
		return;
	
	map_remove(ports, (void*)(uint64_t)port);
}
