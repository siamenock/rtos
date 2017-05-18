#include <net/ether.h>
#include <net/ip.h>
#include <net/port.h>
#include <net/udp.h>

#define UDP_PORT_TABLE		"net.ip.udp.porttable"

bool udp_port_alloc(NIC* nic, uint16_t port) {
	uint8_t* port_map = port_map_get(nic, UDP_PORT_TABLE);
	return port_alloc(port_map, port);
}

bool udp_port_free(NIC* nic, uint16_t port) {
	uint8_t* port_map = port_map_get(nic, UDP_PORT_TABLE);
	return port_free(port_map, port);
}

void udp_pack(Packet* packet, uint16_t udp_body_len) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	UDP* udp = (UDP*)((uint8_t*)ip + ip->ihl * 4);
	
	uint16_t udp_len = UDP_LEN + udp_body_len;
	udp->length = endian16(udp_len);
	udp->checksum = 0;	// TODO: Calc checksum
	
	ip_pack(packet, udp_len);
}
