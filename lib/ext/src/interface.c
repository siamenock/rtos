#include <net/interface.h>
#include <_malloc.h>
#include <string.h>

#define NIC_ADDR_IPv4	"net.addr.ipv4"

IPv4InterfaceTable* interface_table_get(NIC* nic) {
	int32_t interface_key = nic_config_key(nic, NIC_ADDR_IPv4);
	IPv4InterfaceTable* table = NULL;

	if(interface_key < 0) {
		interface_key = nic_config_alloc(nic, NIC_ADDR_IPv4, sizeof(IPv4InterfaceTable));
		if(interface_key < 0)
			return NULL;

		interface_key = nic_config_key(nic, NIC_ADDR_IPv4);

		table = nic_config_get(nic, interface_key);
		memset(table, 0, sizeof(IPv4InterfaceTable));
	} else
		table = nic_config_get(nic, interface_key);

	return table;
}

IPv4Interface* interface_alloc(NIC* nic, uint32_t address, uint32_t netmask, uint32_t gateway, bool is_default) {
	IPv4InterfaceTable* table = interface_table_get(nic);
	if(!table) {
		return NULL;
	}

	if(table->bitmap == 0xff) {
		return NULL;
	}

	IPv4Interface* interface = NULL;
	uint16_t offset = 1;
	for(int i = 0; i < IPV4_INTERFACE_MAX_COUNT; i++) {
		if(!(table->bitmap & offset)) {
			table->bitmap |= offset;
			interface = &table->interfaces[i];
			break;
		}

		offset <<= 1;
	}
	interface->address = address;
	interface->netmask = netmask;
	interface->gateway = gateway;

	if(is_default) {
		//TODO
	}

	return interface;
}

bool interface_free(NIC* nic, uint32_t address) {
	IPv4InterfaceTable* table = interface_table_get(nic);
	if(!table)
		return false;

	if(table->bitmap == 0)
		return false;

	int offset = 1;
	for(int i = 0; i < IPV4_INTERFACE_MAX_COUNT; i++) {
		if((table->bitmap & offset) &&
				address == table->interfaces[i].address) {
			table->bitmap ^= offset;
			return true;
		}

		offset <<= 1;
	}

	return false;
}

IPv4Interface* interface_get_default(NIC* nic) {
	IPv4InterfaceTable* table = interface_table_get(nic);

	if(table->bitmap == 0)
		return NULL;

	return &table->interfaces[table->default_idx];
}

IPv4Interface* interface_get(NIC* nic, uint32_t address) {
	IPv4InterfaceTable* table = interface_table_get(nic);
	if(!table)
		return NULL;

	if(table->bitmap == 0)
		return NULL;

	int offset = 1;
	for(int i = 0; i < IPV4_INTERFACE_MAX_COUNT; i++) {
		if((table->bitmap & offset) &&
				address == table->interfaces[i].address) {
			return &table->interfaces[i];
		}

		offset <<= 1;
	}

	return NULL;
}
