#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <thread.h>
#include <net/ni.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <util/list.h>
#include <util/map.h>
#include <util/event.h>

typedef struct {
	uint32_t	addr;
	uint16_t	port;
} Endpoint;

typedef struct {
	Endpoint	source;
	uint16_t	port;
	Endpoint	destination;
	uint64_t	fin;
} Session;

static Map* sessions;
static Map* ports;

static void rip_add(uint32_t addr, uint16_t port) {
	Endpoint* endpoint = malloc(sizeof(Endpoint));
	endpoint->addr = addr;
	endpoint->port = port;
	
	NetworkInterface* ni_intra = ni_get(1);
	List* rips = ni_config_get(ni_intra, "pn.lb.rips");
	list_add(rips, endpoint);
}

static Endpoint* rip_alloc(NetworkInterface* ni) {
	static int robin;
	
	List* rips = ni_config_get(ni, "pn.lb.rips");
	int idx = (robin++) % list_size(rips);
	
	return list_get(rips, idx);
}

int ginit(int argc, char** argv) {
	uint32_t count = ni_count();
	if(count != 2)
		return 1;
	
	NetworkInterface* ni_inter = ni_get(0);
	ni_config_put(ni_inter, "ip", (void*)(uint64_t)0xc0a8640a);	// 192.168.100.10
	ni_config_put(ni_inter, "pn.lb.port", (void*)(uint64_t)80);
	arp_announce(ni_inter, 0);
	
	NetworkInterface* ni_intra = ni_get(1);
	ni_config_put(ni_intra, "ip", (void*)(uint64_t)0xc0a86414);	// 192.168.100.200
	arp_announce(ni_intra, 0);
	
	List* rips = list_create(NULL);
	ni_config_put(ni_intra, "pn.lb.rips", rips);
	
	rip_add(0xc0a864c8, 8080);	//192.168.100.200
	rip_add(0xc0a864c8, 8081);
	rip_add(0xc0a864c8, 8082);
	rip_add(0xc0a864c8, 8083);
	rip_add(0xc0a864c8, 8084);
	
	sessions = map_create(4096, NULL, NULL, NULL);
	ports = map_create(4096, NULL, NULL, NULL);
	
	return 0;
}

void init(int argc, char** argv) {
	sessions = map_create(4096, NULL, NULL, NULL);
}

static NetworkInterface* ni_inter;
static NetworkInterface* ni_intra;
 
static Session* session_alloc(uint32_t saddr, uint16_t sport) {
	uint64_t key = (uint64_t)saddr << 32 | (uint64_t)sport;
	
	Session* session = malloc(sizeof(Session));
	session->source.addr = saddr;
	session->source.port = sport;
	session->port = tcp_port_alloc(ni_intra);
	Endpoint* rip = rip_alloc(ni_intra);
	session->destination.addr = rip->addr;
	session->destination.port = rip->port;
	session->fin = 0;
	
	map_put(sessions, (void*)key, session);
	map_put(ports, (void*)(uint64_t)session->port, session);
	
	printf("Alloc session %x:%d -> %d -> %x:%d\n", saddr, sport, session->port, rip->addr, rip->port);
	
	return session;
}

static void session_free(Session* session) {
	printf("Session freeing: %d.%d.%d.%d:%d %d %d.%d.%d.%d:%d\n", 
		(session->source.addr) >> 24 & 0xff,
		(session->source.addr) >> 16 & 0xff,
		(session->source.addr) >> 8 & 0xff,
		(session->source.addr) >> 0 & 0xff,
		session->source.port,
		session->port,
		(session->destination.addr) >> 24 & 0xff,
		(session->destination.addr) >> 16 & 0xff,
		(session->destination.addr) >> 8 & 0xff,
		(session->destination.addr) >> 0 & 0xff,
		session->destination.port);
	
	uint64_t key = (uint64_t)session->source.addr << 32 | (uint64_t)session->source.port;
	map_remove(sessions, (void*)key);
	map_remove(ports, (void*)(uint64_t)session->port);
	tcp_port_free(ni_intra, session->port);
	free(session);
}

void process_inter() {
	NetworkInterface* ni = ni_inter;
	
	Packet* packet = ni_input(ni);
	if(!packet)
		return;
	
	if(arp_process(packet))
		return;
	
	if(icmp_process(packet))
		return;
	
	uint32_t addr = (uint32_t)(uint64_t)ni_config_get(ni, "ip");
	uint16_t port = (uint16_t)(uint64_t)ni_config_get(ni, "pn.lb.port");
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_TCP) {
			TCP* tcp = (TCP*)ip->body;
			
			if(endian32(ip->destination) == addr && endian16(tcp->destination) == port) {
				uint32_t saddr = endian32(ip->source);
				uint16_t sport = endian16(tcp->source);
				//uint32_t raddr = (uint32_t)(uint64_t)ni_config_get(ni_intra, "ip");
				uint64_t key = (uint64_t)saddr << 32 | (uint64_t)sport;
				
				Session* session = map_get(sessions, (void*)key);
				if(!session) {
					session = session_alloc(saddr, sport);
				}
				
				/*
				ip->source = endian32(raddr);
				tcp->source = endian16(session->port);
				*/
				ip->destination = endian32(session->destination.addr);
				tcp->destination = endian16(session->destination.port);
				ether->smac = endian48(ni_intra->mac);
				ether->dmac = endian48(arp_get_mac(ni_intra, session->destination.addr));
				tcp_pack(packet, endian16(ip->length) - ip->ihl * 4 - TCP_LEN);
				
				printf("Incoming: %lx %lx %d.%d.%d.%d:%d %d %d.%d.%d.%d:%d\n", 
					endian48(ether->dmac), 
					endian48(ether->smac),
					(endian32(ip->source)) >> 24 & 0xff,
					(endian32(ip->source)) >> 16 & 0xff,
					(endian32(ip->source)) >> 8 & 0xff,
					(endian32(ip->source)) >> 0 & 0xff,
					endian16(tcp->source),
					session->port,
					(endian32(ip->destination)) >> 24 & 0xff,
					(endian32(ip->destination)) >> 16 & 0xff,
					(endian32(ip->destination)) >> 8 & 0xff,
					(endian32(ip->destination)) >> 0 & 0xff,
					endian16(tcp->destination));
				
				if(session->fin && tcp->ack) {
					printf("Ack fin\n");
					event_timer_remove(session->fin);
					session_free(session);
				}
				
				ni_output(ni_intra, packet);
				
				packet = NULL;
			}
		}
	}
	
	if(packet)
		ni_free(packet);
}

void process_intra() {
	NetworkInterface* ni = ni_intra;
	
	Packet* packet = ni_input(ni);
	if(!packet)
		return;
	
	if(arp_process(packet))
		return;
	
	if(icmp_process(packet))
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_TCP) {
			TCP* tcp = (TCP*)ip->body;
			
			Session* session = map_get(ports, (void*)(uint64_t)endian16(tcp->destination));
			if(session) {
				uint32_t addr = (uint32_t)(uint64_t)ni_config_get(ni_inter, "ip");
				uint16_t port = (uint16_t)(uint64_t)ni_config_get(ni_inter, "pn.lb.port");
				
				tcp->source = endian16(port);
				ip->source = endian32(addr);
				tcp->destination = endian16(session->source.port);
				ip->destination = endian32(session->source.addr);
				ether->smac = endian48(ni_inter->mac);
				ether->dmac = endian48(arp_get_mac(ni_inter, endian32(ip->destination)));
				tcp_pack(packet, endian16(ip->length) - ip->ihl * 4 - TCP_LEN);
				
				printf("Outgoing: %lx %lx %d.%d.%d.%d:%d %d %d.%d.%d.%d:%d\n", 
					endian48(ether->dmac), 
					endian48(ether->smac),
					(endian32(ip->source)) >> 24 & 0xff,
					(endian32(ip->source)) >> 16 & 0xff,
					(endian32(ip->source)) >> 8 & 0xff,
					(endian32(ip->source)) >> 0 & 0xff,
					endian16(tcp->source),
					session->port,
					(endian32(ip->destination)) >> 24 & 0xff,
					(endian32(ip->destination)) >> 16 & 0xff,
					(endian32(ip->destination)) >> 8 & 0xff,
					(endian32(ip->destination)) >> 0 & 0xff,
					endian16(tcp->destination));
				
				if(tcp->fin) {
					bool gc(void* context) {
						Session* session = context;
						
						printf("Timeout fin\n");
						session_free(session);
						
						return false;
					}
					
					printf("Fin timer\n");
					session->fin = event_timer_add(gc, session, 3000, 3000);
				}
				
				ni_output(ni_inter, packet);
				
				packet = NULL;
			}
		}
	}
	
	if(packet)
		ni_free(packet);
}

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
	printf("Thread %d bootting\n", thread_id());
	if(thread_id() == 0) {
		int err = ginit(argc, argv);
		if(err != 0)
			return err;
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	ni_inter = ni_get(0);
	ni_intra = ni_get(1);
	event_init();
	
	while(1) {
		if(ni_has_input(ni_inter)) {
			process_inter();
		}
		
		if(ni_has_input(ni_intra)) {
			process_intra();
		}
		
		event_loop();
	}
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
