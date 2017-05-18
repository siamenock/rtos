#include <net/interface.h>
#include <_malloc.h>
#include <string.h>

#define NIC_ADDR_IPv4	"net.addr.ipv4"

IPv4InterfaceTable* interface_map_get(NIC* nic) {
	int32_t interface_key = nic_config_key(nic, NIC_ADDR_IPv4);
	IPv4InterfaceTable* table = NULL;

	if(interface_key <= 0) {
		interface_key = nic_config_alloc(nic, NIC_ADDR_IPv4, sizeof(IPv4InterfaceTable));
		if(interface_key <= 0)
			return false;

		table = nic_config_get(nic, interface_key);
		memset(table, 0, sizeof(IPv4InterfaceTable));
	} else
		table = nic_config_get(nic, interface_key);

	return table;
}

IPv4Interface* interface_alloc(NIC* nic, uint32_t address, uint32_t netmask) {
	IPv4InterfaceTable* table = interface_map_get(nic);
	if(table->count >= IPV4_INTERFACE_MAX_COUNT)
		return false;

	IPv4Interface* interface = &table->interfaces[table->count++];
	interface->address = address;
	interface->netmask = netmask;

	return interface;
}

bool interface_free(NIC* nic, uint32_t address) {
	IPv4InterfaceTable* table = interface_map_get(nic);
	if(!table)
		return false;

	for(int i = 0; i < table->count; i++) {
		if(address == table->interfaces[i].address) {
			if((table->count - 1) == i) {
				table->count--;
				return true;
			}

			table->interfaces[i] = table->interfaces[--table->count];
			return true;
		}
	}

	return false;
}

IPv4Interface* interface_get(NIC* nic, uint32_t address) {
	IPv4InterfaceTable* table = interface_map_get(nic);
	if(!table)
		return false;

	for(int i = 0; i < table->count; i++) {
		if(address == table->interfaces[i].address)
			return &table->interfaces[i];
	}

	return NULL;
}
