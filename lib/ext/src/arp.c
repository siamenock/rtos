#include <timer.h>
#include <string.h>
#include <net/interface.h>
#include <net/ether.h>
#include <net/arp.h>
#include <nic.h>

#define ARP_TABLE		"net.arp.arptable"
#define ARP_TIMEOUT		(uint64_t)14400 * 1000000	// 4 hours  (us)

#define GC_INTERVAL		(uint64_t)10 * 1000000	// 10 secs

//TODO When load application in kernel, do set arp_table.
inline void arp_table_reflash(ARPTable* table, uint64_t current) {
	if(current < table->timeout)
		return;

	for(int i = 0; i < table->entity_count; i++) {
recheck:
		if(table->entity[i].timeout > current)
			continue;

		if(i == table->entity_count - 1) {
			table->entity_count--;
			break;
		} else {
			table->entity[i] = table->entity[--table->entity_count];
			goto recheck;
		}
	}

	table->timeout = current + GC_INTERVAL;
}

ARPTable* arp_get_table(NIC* nic) {
	//TODO Add cache
	static NIC* _nic = NULL;
	static ARPTable* table = NULL;

	uint64_t current = timer_us();
	if(_nic == nic) {
		arp_table_reflash(table, current);
		return table;
	}

	int32_t table_key = nic_config_key(nic, ARP_TABLE);
	if(table_key <= 0) {
		table_key = nic_config_alloc(nic, ARP_TABLE, sizeof(ARPTable));
		if(table_key <= 0)
			return NULL;

		table = nic_config_get(nic, table_key);
		memset(table, 0, sizeof(ARPTable));

		_nic = nic;

		return table;
	}

	table = nic_config_get(nic, table_key);
	arp_table_reflash(table, current);

	_nic = nic;

	return table;
}

// TODO
bool arp_table_destroy(NIC* nic) {
	return true;
}

bool arp_table_update(ARPTable* table, uint64_t mac, uint32_t addr, bool dynamic) {
	uint64_t current = timer_us();
	for(int i = 0; i < table->entity_count; i++) {
		if(table->entity[i].addr == addr) {
			if(!dynamic) {
				table->entity[i].mac = mac;
				table->entity[i].timeout = current + ARP_TIMEOUT;
			} else if(table->entity[table->entity_count].dynamic == dynamic) {
				table->entity[i].mac = mac;
				table->entity[i].timeout = current + ARP_TIMEOUT;
			} else
				return false;

			return true;
		}
	}

	if(table->entity_count >= ARP_ENTITY_MAX_COUNT)
		return false;

	table->entity[table->entity_count].mac = mac;
	table->entity[table->entity_count].addr = addr;
	table->entity[table->entity_count].dynamic = dynamic;
	table->entity[table->entity_count++].timeout = current + ARP_TIMEOUT;

	return true;
}

inline ARPEntity* arp_table_get_by_addr(ARPTable* table, uint32_t addr) {
	for(int i = 0; i < table->entity_count; i++) {
		if(table->entity[i].addr == addr)
			return &table->entity[i];
	}

	return NULL;
}

inline ARPEntity* arp_table_get_by_mac(ARPTable* table, uint64_t mac) {
	for(int i = 0; i < table->entity_count; i++) {
		if(table->entity[i].mac == mac)
			return &table->entity[i];
	}

	return NULL;
}

inline bool arp_table_remove(ARPTable* table, uint32_t addr) {
	return true;
}

uint64_t arp_get_mac(NIC* nic, uint32_t destination, uint32_t source) {
	ARPTable* table = arp_get_table(nic);
	if(!table) {
		arp_request0(nic, destination, source);
		return 0xffffffffffff;
	}

	ARPEntity* entity = arp_table_get_by_mac(table, destination);
	if(!entity) {
		arp_request0(nic, destination, source);
		return 0xffffffffffff;
	}

	return entity->mac;
}

uint32_t arp_get_ip(NIC* nic, uint64_t mac) {
	ARPTable* table = arp_get_table(nic);
	ARPEntity* entity = arp_table_get_by_mac(table, mac);
	if(!entity)
		return 0;

	return entity->addr;
}

bool arp_process(NIC* nic, Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_ARP)
		return false;

	ARP* arp = (ARP*)ether->payload;

	uint32_t addr = endian32(arp->tpa);

	if(!interface_get(nic, addr))
		return false;

	ARPTable* table = arp_get_table(nic);
	switch(endian16(arp->operation)) {
		case 1:	// Request
			if(table)
				arp_table_update(table, ether->smac, arp->spa, true);

			ether->dmac = ether->smac;
			ether->smac = endian48(nic->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(addr);

			nic_tx(nic, packet);
			return true;
		case 2: // Reply
			;
			uint64_t smac = endian48(arp->sha);
			uint32_t sip = endian32(arp->spa);
			if(table)
				arp_table_update(table, smac, sip, true);
			nic_free(packet);
			return true;
	}

	return false;
}

bool arp_request0(NIC* nic, uint32_t destination, uint32_t source) {
	Packet* packet = nic_alloc(nic, sizeof(Ether) + sizeof(ARP));
	if(!packet)
		return false;

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ether->dmac = endian48(0xffffffffffff);
	ether->smac = endian48(nic->mac);
	ether->type = endian16(ETHER_TYPE_ARP);

	ARP* arp = (ARP*)ether->payload;
	arp->htype = endian16(1);
	arp->ptype = endian16(0x0800);
	arp->hlen = endian8(6);
	arp->plen = endian8(4);
	arp->operation = endian16(1);
	arp->sha = endian48(nic->mac);
	arp->spa = endian32(source);
	arp->tha = endian48(0);
	arp->tpa = endian32(destination);

	packet->end = packet->start + sizeof(Ether) + sizeof(ARP);

	return nic_tx(nic, packet);
}

bool arp_request(NIC* nic, uint32_t destination) {
	IPv4InterfaceTable* table = interface_table_get(nic);
	if(!table)
		return false;

	IPv4Interface* interface = NULL;
	int offset = 1;
	for(int i = 0; i < IPV4_INTERFACE_MAX_COUNT; i++) {
		if(table->bitmap & offset) {
			IPv4Interface* _interface = &table->interfaces[i];
			if((_interface->address & _interface->netmask) == (destination & _interface->netmask)) {
				interface = _interface;
				break;
			}
		}

		offset <<= 1;
	}

	if(!interface)
		return false;

	return arp_request0(nic, destination, interface->address);
}

bool arp_announce(NIC* nic, uint32_t source) {
	if(!interface_get(nic, source))
		return false;

	Packet* packet = nic_alloc(nic, sizeof(Ether) + sizeof(ARP));
	if(!packet)
		return false;

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ether->dmac = endian48(0xffffffffffff);
	ether->smac = endian48(nic->mac);
	ether->type = endian16(ETHER_TYPE_ARP);

	ARP* arp = (ARP*)ether->payload;
	arp->htype = endian16(1);
	arp->ptype = endian16(0x0800);
	arp->hlen = endian8(6);
	arp->plen = endian8(4);
	arp->operation = endian16(2);
	arp->sha = endian48(nic->mac);
	arp->spa = endian32(source);
	arp->tha = endian48(0);
	arp->tpa = endian32(source);

	packet->end = packet->start + sizeof(Ether) + sizeof(ARP);

	return nic_tx(nic, packet);
}

void arp_pack(Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ARP* arp = (ARP*)ether->payload;

	packet->end = ((void*)arp + sizeof(ARP)) - (void*)packet->buffer;
}
