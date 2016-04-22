#include <stdio.h>
#include <malloc.h>
#include <thread.h>
#include <net/ni.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <util/list.h>

Packet* process(Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	printf("%x %x %x\n", endian48(ether->dmac), endian48(ether->smac), endian16(ether->type)); 
	
	return packet;
}

void ginit(int argc, char** argv) {
	ni_init(0, 0xc0a86464, 0xffffff00, 0xc0a864c8, true, process);
}

void init(int argc, char** argv) {
}

/*
void process(NetworkInterface* ni) {
	Packet* packet = ni_input(ni);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	if(endian16(ether->type) == ETHER_TYPE_ARP) {
		// ARP response
		ARP* arp = (ARP*)ether->payload;
		if(endian16(arp->operation) == 1 && endian32(arp->tpa) == vip.ip) {
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(vip.ip);
			
			ni_output(ni, packet);
			packet = NULL;
		}
	} else if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;
		
		if(ip->protocol == IP_PROTOCOL_ICMP && endian32(ip->destination) == vip.ip) {
			// Echo reply
			ICMP* icmp = (ICMP*)ip->body;
			
			icmp->type = 0;
			icmp->checksum = 0;
			icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));
			
			ip->destination = ip->source;
			ip->source = endian32(vip.ip);
			ip->ttl = endian8(64);
			ip->checksum = 0;
			ip->checksum = endian16(checksum(ip, ip->ihl * 4));
			
			ether->dmac = ether->smac;
			ether->smac = endian48(ni->mac);
			
			ni_output(ni, packet);
			packet = NULL;
		} else if(ip->protocol == IP_PROTOCOL_TCP) {
			TCP* tcp = (TCP*)ip->body;
			
			uint32_t sip = endian32(ip->source);
			uint16_t sport = endian16(tcp->source);
			
			if(endian32(ip->destination) == vip.ip) {
				if(endian16(tcp->destination) == vip.port) {
					Session* session = NULL;
					ListIterator iter;
					list_iterator_init(&iter, sessions);
					while(list_iterator_has_next(&iter)) {
						Session* s = list_iterator_next(&iter);
						if(s->s.ip == sip && s->s.port == sport) {
							session = s;
							break;
						}
					}
					
					if(!session) {
						session = malloc(sizeof(Session));
						session->s.mac = endian48(ether->smac);
						session->s.ip = sip;
						session->s.port = sport;
						session->port = next_port;
						session->d.mac = rips[robin].mac;
						session->d.ip = rips[robin].ip;
						session->d.port = rips[robin].port;
						
						list_add(sessions, session);
						
						printf("Create session %x:%d -> %d -> %x:%d\n", sip, sport, session->port, rips[robin].ip, rips[robin].port);
						next_port++;
						if(next_port < 49152)
							next_port += 49152;
						
						robin = (robin + 1) % rips_size;
					}
					
					tcp->source = endian16(session->port);
					ip->source = endian32(vip.ip);
					ether->smac = endian48(ni->mac);
					
					tcp->destination = endian16(session->d.port);
					ip->destination = endian32(session->d.ip);
					ether->dmac = endian48(session->d.mac);
					
					TCP_Pseudo pseudo;
					pseudo.source = ip->source;
					pseudo.destination = ip->destination;
					pseudo.padding = 0;
					pseudo.protocol = ip->protocol;
					pseudo.length = endian16(endian16(ip->length) - ip->ihl * 4);
					
					tcp->checksum = 0;
					uint32_t sum = (uint16_t)~checksum(&pseudo, sizeof(pseudo)) + (uint16_t)~checksum(tcp, endian16(ip->length) - ip->ihl * 4);
					while(sum >> 16)
						sum = (sum & 0xffff) + (sum >> 16);
					
					tcp->checksum = endian16(~sum);
					
					ip->checksum = 0;
					ip->checksum = endian16(checksum(ip, ip->ihl * 4));
					
					ni_output(ni, packet);
					
					packet = NULL;
				} else {
					if(!is_from_rip(sip, sport)) {
						ni_free(packet);
						return;
					}
					
					uint16_t dport = endian16(tcp->destination);
					
					Session* session = NULL;
					ListIterator iter;
					list_iterator_init(&iter, sessions);
					while(list_iterator_has_next(&iter)) {
						Session* s = list_iterator_next(&iter);
						
						if(s->port == dport) {
							session = s;
							
							if(tcp->fin) {
								// TODO: Destroy session after 300 seconds
							}
							
							break;
						}
					}
					
					if(!session) {
						printf("Cannot find session from %x:%d to %x:%d\n", sip, sport, endian32(ip->destination), dport);
						ni_free(packet);
						return;
					}
					
					tcp->source = endian16(vip.port);
					ip->source = endian32(vip.ip);
					ether->smac = endian48(ni->mac);
					
					tcp->destination = endian16(session->s.port);
					ip->destination = endian32(session->s.ip);
					ether->dmac = endian48(session->s.mac);
					
					TCP_Pseudo pseudo;
					pseudo.source = ip->source;
					pseudo.destination = ip->destination;
					pseudo.padding = 0;
					pseudo.protocol = ip->protocol;
					pseudo.length = endian16(endian16(ip->length) - ip->ihl * 4);
					
					tcp->checksum = 0;
					uint32_t sum = (uint16_t)~checksum(&pseudo, sizeof(pseudo)) + (uint16_t)~checksum(tcp, endian16(ip->length) - ip->ihl * 4);
					while(sum >> 16)
						sum = (sum & 0xffff) + (sum >> 16);
					
					tcp->checksum = endian16(~sum);
					
					ip->checksum = 0;
					ip->checksum = endian16(checksum(ip, ip->ihl * 4));
					
					ni_output(ni, packet);
					
					packet = NULL;
				}
			}
		}
	}
	
	if(packet)
		ni_free(packet);
}
*/

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	while(1) {
		ni_poll();
	}
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
