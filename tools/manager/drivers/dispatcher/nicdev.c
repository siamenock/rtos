#include <linux/types.h>

#include "nicdev.h"

#define ETHER_MULTICAST		0Xffffffffffff

#define ID_BUFFER_SIZE (MAX_NIC_DEVICE_COUNT * MAX_VNIC_COUNT / 8)
static uint8_t id_map[ID_BUFFER_SIZE];
static int id_alloc() {
	int i, j;
	uint8_t idx;
	for(i = 0; i < ID_BUFFER_SIZE; i++) {
		if(~id_map[i] & 0xff) {
			idx = 1;
			for(j = 0; j < 8; j++) {
				if(!(id_map[i] & idx)) {
					id_map[i] |= idx;
					return i * 8 + j;
				}

				idx <<= 1;
			}
		}
	}

	return -1;
}

static void id_free(int id) {
	int index = id / 8;
	uint8_t idx = 1 << (id % 8);

	id_map[index] |= idx;
	id_map[index] ^= idx;
}

struct _ethhdr {
	uint64_t	h_dest: 48;	/* destination eth addr	*/
	uint64_t	h_source: 48;	/* source ether addr	*/
	__be16		h_proto;	/* packet type ID field	*/
} __attribute__((packed));

static NICDevice* devs[MAX_NIC_DEVICE_COUNT]; //key string

int nicdev_init() {
	return 0;
}

inline bool netdev_name_compare(char* name1, char* name2) {
	for(int i = 0; i < MAX_NIC_NAME_LEN; i++) {
		if(name1[i] != name1[i]) {
			return false;
		}

		if(name1[i] == '\0')
			return true;
	}

	return false;
}

int nicdev_register(NICDevice* dev) {
	//Check name
	int i;
	for(i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!devs[i]) {
			devs[i] = dev;
			return 0;
		}

		if(!netdev_name_compare(devs[i]->name, devs[i]->name))
			return -1;
	}

	return -2;
}

NICDevice* nicdev_unregister(char* name) {
	NICDevice* dev;
	int i, j;
	for(i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!devs[i]) {
			return NULL;
		}

		if(!netdev_name_compare(devs[i]->name, name)) {
			dev = devs[i];
			devs[i] = NULL;
			for(j = i; j + 1 < MAX_NIC_DEVICE_COUNT; j++) {
				if(devs[j + 1]) {
					devs[j] = devs[j + 1];
					devs[j + 1] = NULL;
				}
			}

			return dev;
		}
	}

	return NULL;
}

NICDevice* nicdev_get(char* name) {
	NICDevice* dev;
	int i;
	for(i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!devs[i]) {
			return NULL;
		}

		if(!netdev_name_compare(devs[i]->name, name))
			return dev;
	}

	return NULL;
}

int nicdev_register_vnic(NICDevice* dev, VNIC* vnic) {
	int i;
	for(i = 0; i < MAX_VNIC_COUNT; i++) {
		if(!dev->vnics[i]) {
			dev->vnics[i] = vnic;
			vnic->id = id_alloc();
			return vnic->id;
		}

		if(dev->vnics[i]->mac == vnic->mac)
			return -1;
	}

	return -2;
}

VNIC* nicdev_unregister_vnic(NICDevice* dev, uint32_t id) {
	VNIC* vnic;
	int i, j;

	for(i = 0; i < MAX_VNIC_COUNT; i++) {
		if(!devs[i]) {
			return NULL;
		}

		if(dev->vnics[i]->id == id) {
			//Shift
			vnic = dev->vnics[i];
			dev->vnics[i] = NULL;
			for(j = i; j + 1 < MAX_VNIC_COUNT; j++) {
				if(dev->vnics[j + 1]) {
					dev->vnics[j] = dev->vnics[j + 1];
					dev->vnics[j + 1] = NULL;
				}
			}

			id_free(id);
			return vnic;
		}
	}

	return NULL;
}

VNIC* nicdev_get_vnic(NICDevice* dev, uint32_t id) {
	int i;

	for(i = 0; i < MAX_VNIC_COUNT; i++) {
		if(!devs[i]) {
			return NULL;
		}

		if(dev->vnics[i]->id == id)
			return dev->vnics[i];
	}

	return NULL;
}

VNIC* nicdev_get_vnic_mac(NICDevice* dev, uint64_t mac) {
	int i;

	for(i = 0; i < MAX_VNIC_COUNT; i++) {
		if(!devs[i]) {
			return NULL;
		}

		if(dev->vnics[i]->mac == mac)
			return dev->vnics[i];
	}

	return NULL;
}

VNIC* nicdev_update_vnic(NICDevice* dev, VNIC* src_vnic) {
	VNIC* dst_vnic = nicdev_get_vnic(dev, src_vnic->id);
	if(!dst_vnic)
		return NULL;

	if(dst_vnic->mac != src_vnic->mac) {
		if(nicdev_get_vnic_mac(dev, src_vnic->mac))
			return NULL;

		dst_vnic->mac = src_vnic->mac;
	}

	//TODO fix bandwidth
	dst_vnic->rx_bandwidth = dst_vnic->nic->rx_bandwidth = src_vnic->rx_bandwidth;
	dst_vnic->tx_bandwidth = dst_vnic->nic->tx_bandwidth = src_vnic->tx_bandwidth;
	dst_vnic->padding_head = dst_vnic->nic->padding_head = src_vnic->padding_head;
	dst_vnic->padding_tail = dst_vnic->nic->padding_tail = src_vnic->padding_tail;

	return dst_vnic;
}
/**
 * @param dev NIC device
 * @param data data to be sent
 * @param size data size
 *
 * @return result of process
 */
int nicdev_rx(NICDevice* dev, void* data, size_t size) {
	struct _ethhdr* eth = data;
	int i;
	VNIC* vnic;

	//TODO lock
	if(eth->h_dest == ETHER_MULTICAST) {
		for(i = 0; i < MAX_VNIC_COUNT; i++) {
			if(!dev->vnics[i])
				break;

			vnic_rx(dev->vnics[i], (uint8_t*)eth, size, NULL, 0);
		}
		return NICDEV_PROCESS_PASS;
	} else {

		vnic = nicdev_get_vnic_mac(dev, eth->h_dest);
		if(vnic) {
			vnic_rx(vnic, (uint8_t*)eth, size, NULL, 0);
			return NICDEV_PROCESS_COMPLETE;
		}
	}

	return NICDEV_PROCESS_PASS;
}

/**
 * @param dev NIC device
 * @param process function to process packets in NIC device
 * @param context context to be passed to process function
 *
 * @return number of packets proccessed
 */
int nicdev_tx(NICDevice* dev,
		bool (*process)(Packet* packet, void* context), void* context) {
	Packet* packet;
	VNIC* vnic;
	int budget;
	int i;

	//TODO lock
	for(i = 0; i < MAX_VNIC_COUNT; i++) {
		vnic = dev->vnics[i];
		if(!vnic)
			break;

		budget = vnic->budget;
		while(budget--) {
			packet = vnic_tx(vnic);
			if(!packet)
				break;

			if(!process(packet, context)) {
				nic_free(packet);
				return 0;
			}
		}
	}

	return 0;
}
