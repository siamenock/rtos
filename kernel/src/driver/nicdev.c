#include "gmalloc.h"
#include "nicdev.h"

#define ETHER_TYPE_IPv4		0x0800		///< Ether type of IPv4
#define ETHER_TYPE_ARP		0x0806		///< Ether type of ARP
#define ETHER_TYPE_IPv6		0x86dd		///< Ether type of IPv6
#define ETHER_TYPE_LLDP		0x88cc		///< Ether type of LLDP
#define ETHER_TYPE_8021Q	0x8100		///< Ether type of 802.1q
#define ETHER_TYPE_8021AD	0x88a8		///< Ether type of 802.1ad
#define ETHER_TYPE_QINQ1	0x9100		///< Ether type of QinQ
#define ETHER_TYPE_QINQ2	0x9200		///< Ether type of QinQ
#define ETHER_TYPE_QINQ3	0x9300		///< Ether type of QinQ

#define endian16(v)		__builtin_bswap16((v))	///< Change endianness for 48 bits
#define endian48(v)		(__builtin_bswap64((v)) >> 16)	///< Change endianness for 48 bits

#define ETHER_MULTICAST		((uint64_t)1 << 40)	///< MAC address is multicast
#define ID_BUFFER_SIZE		(MAX_NIC_DEVICE_COUNT * 8)

extern int strncmp(const char* s, const char* d, size_t size);

typedef struct _Ether {
	uint64_t dmac: 48;			///< Destination address (endian48)
	uint64_t smac: 48;			///< Destination address (endian48)
	uint16_t type;				///< Ether type (endian16)
	uint8_t payload[0];			///< Ehternet payload
} __attribute__ ((packed)) Ether;

static NICDevice* nic_devices[MAX_NIC_DEVICE_COUNT]; //key string

int nicdev_get_count() {
	for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!nic_devices[i])
			return i;
	}

	return MAX_NIC_DEVICE_COUNT;
}

NICDevice* nicdev_create() {
	return gmalloc(sizeof(NICDevice));
}

void nicdev_destroy(NICDevice* dev) {
	gfree(dev);
}

int nicdev_register(NICDevice* dev) {
	//Check name
	//
	int i;
	for(i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!nic_devices[i]) {
			nic_devices[i] = dev;
			return 0;
		}

		if(!strncmp(nic_devices[i]->name, dev->name, MAX_NIC_NAME_LEN))
			return -1;
	}

	return -2;
}

NICDevice* nicdev_unregister(const char* name) {
	NICDevice* dev;
	int i, j;
	for(i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!nic_devices[i]) {
			return NULL;
		}

		if(!strncmp(nic_devices[i]->name, name, MAX_NIC_NAME_LEN)) {
			dev = nic_devices[i];
			nic_devices[i] = NULL;
			for(j = i; j + 1 < MAX_NIC_DEVICE_COUNT; j++) {
				if(nic_devices[j + 1]) {
					nic_devices[j] = nic_devices[j + 1];
					nic_devices[j + 1] = NULL;
				}
			}

			return dev;
		}
	}

	return NULL;
}

NICDevice* nicdev_get_by_idx(int idx) {
	if(idx >= MAX_NIC_DEVICE_COUNT)
		return NULL;

	return nic_devices[idx];
}

NICDevice* nicdev_get(const char* name) {
	int i;
	for(i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		if(!nic_devices[i])
			return NULL;

		if(!strncmp(nic_devices[i]->name, name, MAX_NIC_NAME_LEN))
			return nic_devices[i];
	}

	return NULL;
}

int nicdev_poll() {
	int poll_count = 0;
	for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		NICDevice* dev = nic_devices[i];
		if(!dev)
			break;

		NICDriver* driver = dev->driver;
		poll_count += driver->poll(dev->priv);
	}

	return poll_count;
}

int nicdev_register_vnic(NICDevice* dev, VNIC* vnic) {
	int i;
	for(i = 0; i < MAX_VNIC_COUNT; i++) {
		if(!dev->vnics[i]) {
			dev->vnics[i] = vnic;
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
		if(!nic_devices[i])
			return NULL;

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

			return vnic;
		}
	}

	return NULL;
}

VNIC* nicdev_get_vnic(NICDevice* dev, uint32_t id) {
	if(!dev)
		return NULL;

	for(int i = 0; i < MAX_VNIC_COUNT; i++) {
		if(dev->vnics[i]->id == id)
			return dev->vnics[i];
	}

	return NULL;
}

VNIC* nicdev_get_vnic_mac(NICDevice* dev, uint64_t mac) {
	if(!dev)
		return NULL;

	for(int i = 0; i < MAX_VNIC_COUNT; i++) {
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

static uint8_t packet_debug_switch;
void nidev_debug_switch_set(uint8_t opt) {
	packet_debug_switch = opt;
}

uint8_t nicdev_debug_switch_get() {
	return packet_debug_switch;
}

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

inline static void packet_dump(void* _data, size_t size) {
	if(unlikely(!!packet_debug_switch)) {
		if(packet_debug_switch | NICDEV_DEBUG_PACKET_INFO) {
			printf("Packet Lengh:\t%d\n", size);
		}

		Ether* eth = _data;
		if(packet_debug_switch | NICDEV_DEBUG_PACKET_ETHER_INFO) {
			printf("Ether Type:\t0x%04x\n", endian16(eth->type));
			uint64_t dmac = endian48(eth->dmac);
			uint64_t smac = endian48(eth->smac);
			printf("%02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
					(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
					(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
					(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
					(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
		}

		if(packet_debug_switch | NICDEV_DEBUG_PACKET_VERBOSE_INFO) {
			switch(eth->type) {
				case ETHER_TYPE_IPv4:
					break;
				case ETHER_TYPE_ARP:
					break;
				case ETHER_TYPE_IPv6:
					break;
				case ETHER_TYPE_LLDP:
					break;
				case ETHER_TYPE_8021Q:
					break;
				case ETHER_TYPE_8021AD:
					break;
				case ETHER_TYPE_QINQ1:
					break;
				case ETHER_TYPE_QINQ2:
					break;
				case ETHER_TYPE_QINQ3:
					break;
			}
		}

		if(packet_debug_switch | NICDEV_DEBUG_PACKET_DUMP) {
			uint8_t* data = (uint8_t*)_data;
			for(int i = 0 ; i < size;) {
				printf("\t0x%04x:\t", i);
				for(int j = 0; j < 16 && i < size; j++, i++) {
					printf("%02x ", data[i] & 0xff);
				}
				printf("\n");
			}
			printf("\n");
		}
	}
}

int nicdev_rx(NICDevice* dev, void* data, size_t size) {
	return nicdev_rx0(dev, data, size, NULL, 0);
}

int nicdev_rx0(NICDevice* dev, void* data, size_t size,
		void* data_optional, size_t size_optional) {
	Ether* eth = data;
	int i;
	VNIC* vnic;
	uint64_t dmac = endian48(eth->dmac);

	//TODO lock
	packet_dump(data, size);
	if(dmac & ETHER_MULTICAST) {
		for(i = 0; i < MAX_VNIC_COUNT; i++) {
			if(!dev->vnics[i])
				break;

			vnic_rx(dev->vnics[i], (uint8_t*)eth, size, data_optional, size_optional);
		}
		return NICDEV_PROCESS_PASS;
	} else {
		vnic = nicdev_get_vnic_mac(dev, dmac);
		if(vnic) {
			vnic_rx(vnic, (uint8_t*)eth, size, data_optional, size_optional);
			return NICDEV_PROCESS_COMPLETE;
		}
	}

	return NICDEV_PROCESS_PASS;
}

typedef struct _TransmitContext{
	bool (*process)(Packet* packet, void* context);
	void* context;
} TransmitContext;

static bool transmitter(Packet* packet, void* context) {
	if(!packet)
		return false;

	packet_dump(packet->buffer + packet->start, packet->end - packet->start);
	TransmitContext* transmitter_context = context;

	if(!transmitter_context->process(packet, transmitter_context->context))
		return false;

	return true;
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
	int count = 0;

	TransmitContext transmitter_context = {
		.process = process,
		.context = context};

	//TODO lock
	for(int i = 0; i < MAX_VNIC_COUNT; i++) {
		vnic = dev->vnics[i];
		if(!vnic)
			break;

		budget = vnic->budget;
		while(budget--) {
			VNICError ret = vnic_tx(vnic, transmitter, &transmitter_context);

			if(ret == VNIC_ERROR_OPERATION_FAILED) // Transmiitter Error
				return count;
			else if(ret == VNIC_ERROR_RESOURCE_NOT_AVAILABLE) // There no Packet, Check next vnic
				break;

			count++;
		}
	}

	return count;
}

void nicdev_free(Packet* packet) {
	// TODO
	for(int i = 0; i < MAX_VNIC_COUNT; ++i) {
	}
}
