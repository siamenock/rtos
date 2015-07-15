#include <time.h>
#include <net/interface.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/udp.h>
#include <util/map.h>

bool udp_port_alloc0(NetworkInterface* ni, uint32_t addr, uint16_t port) {
	IPv4Interface* interface = ni_ip_get(ni, addr);
	if(!interface->udp_ports) {
		interface->udp_ports = set_create(64, set_uint64_hash, set_uint64_equals, ni->pool);
		if(!interface->udp_ports)
			return false;
	}

	if(set_contains(interface->udp_ports, (void*)(uintptr_t)port))
		return false;

	return set_put(interface->udp_ports, (void*)(uintptr_t)port);
}

uint16_t udp_port_alloc(NetworkInterface* ni, uint32_t addr) {
	IPv4Interface* interface = ni_ip_get(ni, addr);
	if(!interface->udp_ports) {
		interface->udp_ports = set_create(64, set_uint64_hash, set_uint64_equals, ni->pool);
		if(!interface->udp_ports)
			return 0;
	}

	uint16_t port = interface->udp_next_port;
	if(port < 49152)
		port = 49152;

	while(set_contains(interface->udp_ports, (void*)(uintptr_t)port)) {
		if(++port < 49152)
			port = 49152;
	}	

	if(!set_put(interface->udp_ports, (void*)(uintptr_t)port))
		return 0;

	interface->udp_next_port = port;

	return port;
}

void udp_port_free(NetworkInterface* ni, uint32_t addr, uint16_t port) {
	IPv4Interface* interface = ni_ip_get(ni, addr);
	if(interface == NULL)
		return;
	
	set_remove(interface->udp_ports, (void*)(uintptr_t)port);
}

void udp_pack(Packet* packet, uint16_t udp_body_len) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	UDP* udp = (UDP*)ip->body;
	
	uint16_t udp_len = UDP_LEN + udp_body_len;
	udp->length = endian16(udp_len);
	udp->checksum = 0;	// TODO: Calc checksum
	
	ip_pack(packet, udp_len);
}
