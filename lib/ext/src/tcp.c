#include <net/ether.h>
#include <net/ip.h>
#include <net/port.h>
#include <net/tcp.h>
#include <net/checksum.h>

#define TCP_PORT_TABLE		"net.ip.tcp.porttable"

bool tcp_port_alloc(NIC* nic, uint16_t port) {
	uint8_t* port_map = port_map_get(nic, TCP_PORT_TABLE);
	return port_alloc(port_map, port);
}

bool tcp_port_free(NIC* nic, uint16_t port) {
	uint8_t* port_map = port_map_get(nic, TCP_PORT_TABLE);
	return port_free(port_map, port);
}

void tcp_pack(Packet* packet, uint16_t tcp_body_len) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	TCP* tcp = (TCP*)((uint8_t*)ip + ip->ihl * 4);
	
	uint16_t tcp_len = (tcp->offset * 4) + tcp_body_len;
	
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
