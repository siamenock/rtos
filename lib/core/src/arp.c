#include <time.h>
#include <string.h>
#include <malloc.h>
#include <net/ether.h>
#include <net/arp.h>
#include <util/map.h>

#define ARP_TABLE	"net.arp.arptable"
#define ARP_TABLE_GC	"net.arp.arptable.gc"

#define GC_INTERVAL	(10 * 1000000)	// 10 secs

typedef struct {
	uint64_t	mac;
	uint64_t	timeout;
} ARPEntity;

uint32_t ARP_TIMEOUT = 14400;	// 4 hours

bool arp_process(Packet* packet) {
	Map* config = packet->ni->config;
	if(!config)
		return false;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_ARP)
		return false;
	
	uint32_t ip = (uint32_t)(uint64_t)map_get(config, "ip");
	if(!ip)
		return false;
	
	Map* arp_table = map_get(config, ARP_TABLE);
	if(!arp_table) {
		arp_table = map_create(32, map_uint64_hash, map_uint64_equals, config->malloc, config->free);
		map_put(config, strdup(ARP_TABLE), arp_table);
	}
	
	clock_t current = clock();
	
	// GC
	uint64_t gc_time = (uint64_t)map_get(config, ARP_TABLE_GC);
	if(gc_time == 0) {
		gc_time = current + GC_INTERVAL;
		map_put(config, strdup(ARP_TABLE_GC), (void*)gc_time);
	}
	
	if(gc_time < current) {
		MapIterator iter;
		map_iterator_init(&iter, arp_table);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			if(((ARPEntity*)entry->data)->timeout < current) {
				free(entry->key);	// strdup
				arp_table->free(entry->data);
				
				map_iterator_remove(&iter);
			}
		}
		
		gc_time = current + GC_INTERVAL;
		map_update(config, ARP_TABLE_GC, (void*)gc_time);
	}
	
	ARP* arp = (ARP*)ether->payload;
	switch(endian16(arp->operation)) {
		case 1:	// Request
			if(endian32(arp->tpa) == ip) {
				ether->dmac = ether->smac;
				ether->smac = endian48(packet->ni->mac);
				arp->operation = endian16(2);
				arp->tha = arp->sha;
				arp->tpa = arp->spa;
				arp->sha = ether->smac;
				arp->spa = endian32(ip);
				
				ni_output(packet->ni, packet);
				
				return true;
			}
			break;
		case 2: // Reply
			;
			uint64_t smac = endian48(arp->sha);
			uint32_t sip = endian32(arp->spa);
			ARPEntity* entity = map_get(arp_table, (void*)(uint64_t)ip);
			if(!entity) {
				entity = arp_table->malloc(sizeof(ARPEntity));
				entity->mac = smac;
				entity->timeout = current + ARP_TIMEOUT;
				map_put(arp_table, (void*)(uint64_t)sip, entity);
			}
			entity->mac = smac;
			entity->timeout = current + ARP_TIMEOUT;
			return true;
	} 
	
	return false;
}

uint64_t arp_get_mac(NetworkInterface* ni, uint32_t ip) {
	Map* config = ni->config;
	if(!config)
		return 0;
	
	Map* arp_table = map_get(config, ARP_TABLE);
	if(!arp_table) {
		return 0;
	}
	
	ARPEntity* entity = map_get(arp_table, (void*)(uint64_t)ip);
	if(!entity)
		return 0;
	
	return entity->mac;
}

uint32_t arp_get_ip(NetworkInterface* ni, uint64_t mac) {
	Map* config = ni->config;
	if(!config)
		return 0;
	
	Map* arp_table = map_get(config, ARP_TABLE);
	if(!arp_table) {
		return 0;
	}
	
	MapIterator iter;
	map_iterator_init(&iter, arp_table);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		if(((ARPEntity*)entry->data)->mac == mac)
			return (uint32_t)(uint64_t)entry->key;
	}
	
	return 0;
}
