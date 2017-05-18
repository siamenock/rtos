#include <net/port.h>
#include <nic.h>

#define PORT_MAP_SIZE		(65536 / 8)

uint8_t* port_map_get(NIC* nic, char* key_name) {
	int32_t port_map_key = nic_config_key(nic, key_name);
	uint8_t* port_map = NULL;

	if(port_map_key <= 0) {
		port_map_key = nic_config_alloc(nic, key_name, PORT_MAP_SIZE);
		if(port_map_key <= 0)
			return false;

		port_map = nic_config_get(nic, port_map_key);
		memset(port_map, 0, PORT_MAP_SIZE);
	} else
		port_map = nic_config_get(nic, port_map_key);

	return port_map;
}

bool port_free(uint8_t* port_map, uint16_t port) {
	int index = port / 8;
	uint8_t idx = 1 << (port % 8);

	port_map[index] |= idx;
	port_map[index] ^= idx;

	return true;
}

static bool port_alloc0(uint8_t* port_map, uint16_t port) {
	int index = port / 8;
	uint8_t idx = 1 << (port % 8);

	if(port_map[index] & idx)
		return false;

	port_map[index] |= idx;

	return true;
}

uint16_t port_alloc(uint8_t* port_map, uint16_t port) {
	if(port)
		return port_alloc0(port_map, port);

	for(int i = 1024; i < 49152; i++) {
		if(port_alloc0(port_map, i))
			return i;
	}

	return 0;
}
