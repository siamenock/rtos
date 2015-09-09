#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <tlsf.h>
#include <timer.h>
#include <util/types.h>
#include <net/ether.h>
#include <net/vlan.h>
#include <net/ip.h>
#include <util/map.h>
#include <util/event.h>
#include "gmalloc.h"
#include "mp.h"
#include "pci.h"
#include "driver/nic.h"
#include "device.h"
#include <stdlib.h>

#include "vnic.h"

#define DEBUG		0
#define ALIGN		16

Device* nic_current;

uint64_t nic_rx;
uint64_t nic_tx;
uint64_t nic_tx2;

Device* nic_parse_index(char* _argv, uint16_t* port) {
	if(strncmp(_argv, "eth", 3)) {
		return NULL;
	}

	char* next;
	*port = strtol(_argv + 3, &next, 0);
	if(next == _argv + 3) {
		return NULL;
	}

	if(*next != '.' && *next != '\0')
		return NULL;

	Device* dev = NULL;
	for(int i = 0; i < NIC_MAX_DEVICE_COUNT; i++) {
		if(*port < ((NICInfo*)nic_devices[i]->priv)->port_count) { 
			dev = nic_devices[i];
			break;
		} else {
			*port -= ((NICPriv*)nic_devices[i]->priv)->port_count;
		}
	}
	if(!dev)
		return NULL;

	uint16_t vid = 0;
	if(*next == '.') {
		next++;

		if(!is_uint16(next))
			return NULL;

		vid = parse_uint16(next);
	}

	*port = (*port << 12) | vid;

	return dev;
}

static bool vlan_input_process(VNIC* vnic, Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	uint16_t type = endian16(ether->type);
	if(type == ETHER_TYPE_8021Q) {
		VLAN* vlan = (VLAN*)ether->payload;
		uint16_t vid = VLAN_GET_VID(endian16(vlan->tci));
		if((vnic->port & 0xfff) != vid) {
			return false;
		}

		memmove((uint8_t*)ether + 4 , ether, ETHER_LEN - 2);
		packet->start += 4;

		return true;
	}

	return false;
}

static bool vlan_output_process(VNIC* vnic, Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	//check start point
	if(packet->start < 4) {
		return false;
	}

	memmove((uint8_t*)ether - 4, ether, ETHER_LEN - 2);
	packet->start -= 4;
	ether = (void*)(packet->buffer + packet->start);
	ether->type = endian16(ETHER_TYPE_8021Q);
	VLAN* vlan = (VLAN*)ether->payload;
	vlan->tci = endian16(vnic->port & 0xfff);

	return true;
}

void vnic_init0() {
}

/**
 *
 * Error: errno
 * 1: mandatory attributes not specified
 * 2: Conflict attributes speicifed
 * 3: Pool size is not multiple of 2MB
 * 4: Not enough memory
 */
VNIC* vnic_create(uint64_t* attrs) {
	bool has_key(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != NI_NONE) {
			if(attrs[i * 2] == key)
				return true;

			i++;
		}

		return false;
	}

	uint64_t get_value(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != NI_NONE) {
			if(attrs[i * 2] == key)
				return attrs[i * 2 + 1];

			i++;
		}

		return (uint64_t)-1;
	}

	errno = 0;

	if(!has_key(NI_DEV) || !has_key(NI_MAC) || !has_key(NI_POOL_SIZE) || 
			!has_key(NI_INPUT_BANDWIDTH) || !has_key(NI_OUTPUT_BANDWIDTH) ||
			!has_key(NI_INPUT_BUFFER_SIZE) || !has_key(NI_OUTPUT_BUFFER_SIZE)) {
		errno = 1;
		return NULL;
	}

	if((has_key(NI_INPUT_ACCEPT_ALL) && has_key(NI_INPUT_ACCEPT)) ||
			(has_key(NI_OUTPUT_ACCEPT_ALL) && has_key(NI_OUTPUT_ACCEPT))) {
		errno = 2;
		return NULL;
	}

	Device* dev = NULL;
	uint16_t port = 0;
	Map* vnics;
	char* name = (char*)get_value(NI_DEV);
	if(name) {
		dev = nic_parse_index(name, &port);
		if(!dev) {
			errno = 1;
			return NULL;
		}

		vnics = map_get(((NICPriv*)dev->priv)->nics, (void*)(uint64_t)port);
		if(!vnics) {
			errno = 1;
			return NULL;
		}

		uint64_t mac = get_value(NI_MAC);
		if(map_contains(vnics, (void*)mac)) {
			errno = 1;
			return NULL;
		}
	} else {
		errno = 1;
		return NULL;
	}

	// Allocate NI
	VNIC* vnic = gmalloc(sizeof(VNIC));
	if(!vnic)
		return NULL;
	bzero(vnic, sizeof(VNIC));
	// Allocate initial pool
	uint64_t pool_size = get_value(NI_POOL_SIZE);
	if(pool_size == 0 || (pool_size & ~(0x200000 - 1)) != pool_size) {
		gfree(vnic);
		errno = 4;
		return NULL;
	}

	void* pool = bmalloc();
	if(pool == NULL) {
		gfree(vnic);
		errno = 5;
		return NULL;
	}

	init_memory_pool(0x200000, pool, 1);
	vnic->pool = pool;

	// Allocate extra pools
	vnic->pools = list_create(vnic->pool);
	if(!vnic->pools) {
		gfree(vnic);
		errno = 5;
		return NULL;
	}
	list_add(vnic->pools, pool);
	uint32_t pool_count = pool_size / 0x200000 - 1;
	for(uint32_t i = 0; i < pool_count; i++) {
		void* ptr = bmalloc();

		if(!ptr) {
			while(!list_is_empty(vnic->pools)) {
				ptr = list_remove_first(vnic->pools);
				bfree(ptr);
			}
			gfree(vnic);
			errno = 6;

			return NULL;
		}

		list_add(vnic->pools, ptr);
		add_new_area(ptr, 0x200000, pool);
	}

	// Allocate NetworkInterface
	vnic->device = dev;
	vnic->port = port;

	vnic->ni = malloc_ex(sizeof(NetworkInterface), vnic->pool);
	bzero(vnic->ni, sizeof(NetworkInterface));

	vnic->ni->min_buffer_size = vnic->min_buffer_size = 128;
	vnic->ni->max_buffer_size = vnic->max_buffer_size = 2048;
	if(!has_key(NI_INPUT_ACCEPT_ALL))
		vnic->input_accept = list_create(vnic->pool);
	if(!has_key(NI_OUTPUT_ACCEPT_ALL))
		vnic->output_accept = list_create(vnic->pool);

	vnic->input_closed = vnic->output_closed = timer_frequency();

	// Extra attributes
	int i = 0;
	while(attrs[i * 2] != NI_NONE) {
		switch(attrs[i * 2]) {
			case NI_MAC:
				vnic->ni->mac = vnic->mac = attrs[i * 2 + 1];
				break;
			case NI_POOL_SIZE:
				break;
			case NI_INPUT_BANDWIDTH:
				vnic->ni->input_bandwidth = vnic->input_bandwidth = attrs[i * 2 + 1];
				vnic->input_wait = TIMER_FREQUENCY_PER_SEC * 8 / vnic->input_bandwidth;
				vnic->input_wait_grace = TIMER_FREQUENCY_PER_SEC / 100;
				break;
			case NI_OUTPUT_BANDWIDTH:
				vnic->ni->output_bandwidth = vnic->output_bandwidth = attrs[i * 2 + 1];
				vnic->output_wait = TIMER_FREQUENCY_PER_SEC * 8 / vnic->output_bandwidth;
				vnic->output_wait_grace = TIMER_FREQUENCY_PER_SEC / 1000;
				break;
			case NI_PADDING_HEAD:
				vnic->ni->padding_head = vnic->padding_head = attrs[i * 2 + 1];
				break;
			case NI_PADDING_TAIL:
				vnic->ni->padding_tail = vnic->padding_tail = attrs[i * 2 + 1];
				break;
			case NI_INPUT_BUFFER_SIZE:
				vnic->ni->input_buffer = fifo_create(attrs[i * 2 + 1], vnic->pool);
				if(!vnic->ni->input_buffer) {
					errno = 5;
					gfree(vnic);
					return NULL;
				}
				break;
			case NI_OUTPUT_BUFFER_SIZE:
				vnic->ni->output_buffer = fifo_create(attrs[i * 2 + 1], vnic->pool);
				if(!vnic->ni->output_buffer) {
					errno = 5;
					gfree(vnic);
					return NULL;
				}
				break;
			case NI_MIN_BUFFER_SIZE:
				vnic->ni->min_buffer_size = vnic->min_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_MAX_BUFFER_SIZE:
				vnic->ni->max_buffer_size = vnic->max_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_INPUT_ACCEPT:
				if(vnic->input_accept) {
					if(!list_add(vnic->input_accept, (void*)attrs[i * 2 + 1])) {
						errno = 5;
						gfree(vnic);
						return NULL;
					}
				}
				break;
			case NI_OUTPUT_ACCEPT:
				if(vnic->output_accept) {
					if(!list_add(vnic->output_accept, (void*)attrs[i * 2 + 1])) {
						errno = 5;
						gfree(vnic);
						return NULL;
					}
				}
				break;
		}

		i++;
	}
	vnic->ni->pool = vnic->pool;

	// Config
	vnic->ni->config = map_create(8, NULL, NULL, vnic->pool);

	// Register the vnic
	map_put(vnics, (void*)vnic->mac, vnic);

	return vnic;
}

void vnic_destroy(VNIC* vnic) {
	Map* nics = ((NICPriv*)vnic->device->priv)->nics;
	Map* vnics = map_get(nics, (void*)(uint64_t)vnic->port);
	// Unregister the ni
	map_remove(vnics, (void*)vnic->mac);

	// Free pools
	ListIterator iter;
	list_iterator_init(&iter, vnic->pools);

	while(list_iterator_has_next(&iter)) {
		void* pool = list_iterator_next(&iter);
		bfree(pool);
	}

	gfree(vnic);
}

bool vnic_contains(char* ni_dev, uint64_t mac) {
	if(!ni_dev)
		return false;
	
	uint16_t port = 0;
	Device* dev = nic_parse_index(ni_dev, &port);
	if(!dev) {
		return false;
	}

	Map* nics = ((NICPriv*)dev->priv)->nics;
	Map* vnics = map_get(nics, (void*)(uint64_t)port);
	if(!vnics)
		return false;

	return map_contains(vnics, (void*)mac);
}

uint32_t vnic_update(VNIC* vnic, uint64_t* attrs) {
	bool has_key(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != NI_NONE) {
			if(attrs[i * 2] == key)
				return true;
			

			i++;
		}

		return false;
	}

	uint64_t get_value(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != NI_NONE) {
			if(attrs[i * 2] == key)
				return attrs[i * 2 + 1];

			i++;
		}

		return (uint64_t)-1;
	}

	uint32_t result = 0;

	List* pools = NULL;
	void* input_buffer = NULL;
	uint64_t input_buffer_size = 0;
	void* output_buffer = NULL;
	uint64_t output_buffer_size = 0;
	List* input_accept = NULL;
	List* output_accept = NULL;

	int i = 0;
	while(attrs[i * 2] != NI_NONE) {
		switch(attrs[i * 2]) {
			case NI_MAC:
			case NI_DEV:
				;
				if(attrs[i * 2] == NI_DEV) {
					if(has_key(NI_MAC)) {
						//do not
						break;
					}
				}

				Device* dev = NULL;
				uint16_t port;
				if(has_key(NI_DEV)) {
					char* name = (char*)get_value(NI_DEV);
					dev = nic_parse_index(name, &port);
				} else {
					dev = vnic->device;
					port = vnic->port;
				}
				if(!dev) {
					result = 1;
					goto failed;
				}

				Map* nics = ((NICPriv*)dev->priv)->nics;
				Map* vnics = map_get(nics, (void*)(uint64_t)port);
				uint64_t mac;
				if(has_key(NI_MAC)) {
					mac = attrs[i * 2 + 1];
				} else {
					mac = vnic->mac;
				}

				if(map_contains(vnics, (void*)mac)) {
					result = 1;
					goto failed;
				}
				break;
			case NI_POOL_SIZE:
				;
				uint64_t pool_size = attrs[i * 2 + 1];
				if(pool_size < list_size(vnic->pools) * 0x200000) {
					result = 2;
					goto failed;
				}

				if((pool_size & ~(0x200000 - 1)) != pool_size) {
					result = 3;
					goto failed;
				}

				int32_t pools_count = pool_size / 0x200000 - list_size(vnic->pools);
				if(pools_count < 0) {
					result = 4;
					goto failed;
				}

				pools = list_create(vnic->pool);
				if(!pools) {
					result = 5;
					goto failed;
				}

				for(uint32_t i = 0; i < pools_count; i++) {
					void* ptr = bmalloc();
					if(ptr == NULL) {
						result = 6;
						goto failed;
					}

					list_add(pools, ptr);
				}
				break;
			case NI_INPUT_BANDWIDTH:
				break;
			case NI_OUTPUT_BANDWIDTH:
				break;
			case NI_PADDING_HEAD:
				break;
			case NI_PADDING_TAIL:
				break;
			case NI_INPUT_BUFFER_SIZE:
				input_buffer_size = attrs[i * 2 + 1];
				input_buffer = malloc_ex(sizeof(void*) * input_buffer_size, vnic->pool);
				if(!input_buffer) {
					result = 5;
					goto failed;
				}
				break;
			case NI_OUTPUT_BUFFER_SIZE:
				output_buffer_size = attrs[i * 2 + 1];
				output_buffer = malloc_ex(sizeof(void*) * output_buffer_size, vnic->pool);
				if(!output_buffer) {
					result = 5;
					goto failed;
				}
				break;
			case NI_MIN_BUFFER_SIZE:
				break;
			case NI_MAX_BUFFER_SIZE:
				break;
			case NI_INPUT_ACCEPT:
				if(!input_accept) {
					input_accept = list_create(vnic->pool);
					if(!input_accept) {
						result = 5;
						goto failed;
					}
				}

				if(!list_add(input_accept, (void*)attrs[i * 2 + 1])) {
					result = 5;
					goto failed;
				}
				break;
			case NI_OUTPUT_ACCEPT:
				if(!output_accept) {
					output_accept = list_create(vnic->pool);
					if(!output_accept) {
						result = 5;
						goto failed;
					}
				}

				if(!list_add(output_accept, (void*)attrs[i * 2 + 1])) {
					result = 5;
					goto failed;
				}
				break;
		}

		i++;
	}

	goto succeed;

failed:
	if(pools) {
		ListIterator iter;

		list_iterator_init(&iter, pools);
		while(list_iterator_has_next(&iter)) {
			void* ptr = list_iterator_next(&iter);
			bfree(ptr);
		}
		list_destroy(pools);
	}

	if(input_buffer)
		ni_free(input_buffer);

	if(output_buffer)
		ni_free(output_buffer);

	if(input_accept)
		list_destroy(input_accept);

	if(output_accept)
		list_destroy(output_accept);

	return result;

succeed:

	// NI_POOL_SIZE
	if(pools) {
		ListIterator iter;

		list_iterator_init(&iter, pools);
		while(list_iterator_has_next(&iter)) {
			void* ptr = list_iterator_remove(&iter);
			add_new_area(ptr, 0x200000, vnic->pool);
		}
		list_destroy(pools);
		vnic->ni->pool_size = list_size(vnic->pools) * 0x200000;
	}

	// NI_INPUT_BUFFER_SIZE
	if(input_buffer) {
		void input_popped(void* data) {
			Packet* packet = data;

			uint16_t size = packet->end - packet->start;
			vnic->ni->input_drop_bytes += size;
			vnic->ni->input_drop_packets++;

			ni_free(packet);
		}

		void* array = vnic->ni->input_buffer->array;
		fifo_reinit(vnic->ni->input_buffer, input_buffer, input_buffer_size, input_popped);
		free_ex(array, vnic->pool);
	}

	// NI_OUTPUT_BUFFER_SIZE
	if(output_buffer) {
		void output_popped(void* data) {
			Packet* packet = data;

			vnic->ni->output_drop_bytes += packet->end - packet->start;
			vnic->ni->output_drop_packets++;

			free_ex(packet->buffer, vnic->pool);
			free_ex(packet, vnic->pool);
		}

		void* array = vnic->ni->output_buffer->array;
		fifo_reinit(vnic->ni->output_buffer, output_buffer, output_buffer_size, output_popped);
		free_ex(array, vnic->pool);
	}

	// NI_INPUT_ACCEPT
	if(input_accept) {
		if(!vnic->input_accept) {
			vnic->input_accept = input_accept;
		} else {
			while(list_size(input_accept) > 0) {
				list_add(vnic->input_accept, list_remove_first(input_accept));
			}
			list_destroy(input_accept);
		}
	}

	// NI_OUTPUT_ACCEPT
	if(output_accept) {
		if(!vnic->output_accept) {
			vnic->output_accept = output_accept;
		} else {
			while(list_size(output_accept) > 0) {
				list_add(vnic->output_accept, list_remove_first(output_accept));
			}
			list_destroy(output_accept);
		}
	}

	i = 0;
	while(attrs[i * 2] != NI_NONE) {
		switch(attrs[i * 2]) {
			case NI_MAC:
			case NI_DEV:
				;
				if(attrs[i * 2] == NI_DEV) {
					if(has_key(NI_MAC)) {
						//do not
						break;
					}
				}

				Map* nics = ((NICPriv*)vnic->device->priv)->nics;
				Map* vnics = map_get(nics, (void*)(uint64_t)vnic->port);
				map_remove(vnics, (void*)vnic->mac);

				Device* dev = NULL;
				uint16_t port;
				if(has_key(NI_DEV)) {
					char* name = (char*)get_value(NI_DEV);
					dev = nic_parse_index(name, &port);
					vnic->device = dev;
					vnic->port = port;
				} else {
					dev = vnic->device;
					port = vnic->port;
				}
				nics = ((NICPriv*)dev->priv)->nics;
				vnics = map_get(nics, (void*)(uint64_t)port);
				uint64_t mac;
				if(has_key(NI_MAC)) {
					mac = attrs[i * 2 + 1];
					vnic->ni->mac = vnic->mac = mac;
				} else {
					mac = vnic->mac;
				}
				map_put(vnics, (void*)mac, vnic);

				break;
			case NI_INPUT_BANDWIDTH:
				vnic->ni->input_bandwidth = vnic->input_bandwidth = attrs[i * 2 + 1];
				vnic->input_wait = TIMER_FREQUENCY_PER_SEC * 8 / vnic->input_bandwidth;
				vnic->input_wait_grace = TIMER_FREQUENCY_PER_SEC / 100;
				break;
			case NI_OUTPUT_BANDWIDTH:
				vnic->ni->output_bandwidth = vnic->output_bandwidth = attrs[i * 2 + 1];
				vnic->output_wait = TIMER_FREQUENCY_PER_SEC * 8 / vnic->output_bandwidth;
				vnic->output_wait_grace = TIMER_FREQUENCY_PER_SEC / 1000;
				break;
			case NI_PADDING_HEAD:
				vnic->padding_head = attrs[i * 2 + 1];
				break;
			case NI_PADDING_TAIL:
				vnic->padding_tail = attrs[i * 2 + 1];
				break;
			case NI_MIN_BUFFER_SIZE:
				vnic->min_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_MAX_BUFFER_SIZE:
				vnic->max_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_INPUT_ACCEPT_ALL:
				if(vnic->input_accept) {
					list_destroy(vnic->input_accept);
					vnic->input_accept = NULL;
				}
				break;
			case NI_OUTPUT_ACCEPT_ALL:
				if(vnic->output_accept) {
					list_destroy(vnic->output_accept);
					vnic->output_accept = NULL;
				}
				break;
		}
		i++;
	}

	return 0;
}

void nic_process_input(uint8_t local_port, uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2) {
	// Get dmac, smac
	uint64_t dmac;
	uint64_t smac;
	uint16_t vid = 0;

	if(size1 >= 14) {
		Ether* ether = (Ether*)buf1;
		dmac = endian48(ether->dmac);
		smac = endian48(ether->smac);
		if(endian16(ether->type) == ETHER_TYPE_8021Q) {
			if(size1 >= 16) {
				VLAN* vlan = (VLAN*)ether->payload;
				vid = VLAN_GET_VID(endian16(vlan->tci));
			} else {
				VLAN vlan;
				uint8_t* p = (uint8_t*)&vlan;
				ssize_t _size = size1 - 14;
				memcpy(p, buf1, _size);
				memcpy(p + _size, buf2, 2 - _size);
				vid = VLAN_GET_VID(endian16(vlan.tci));
			}
		}
	} else {
		Ether ether;
		uint8_t* p = (uint8_t*)&ether;
		memcpy(p, buf1, size1);

		ssize_t _size = 14 - size1;
		memcpy(p + size1, buf2, _size);
		dmac = endian48(ether.dmac);
		smac = endian48(ether.smac);
		if(endian16(ether.type) == ETHER_TYPE_8021Q) {
			VLAN vlan;
			p = (uint8_t*)&vlan;
			memcpy(p, buf2 + _size, 2);
			vid = VLAN_GET_VID(endian16(vlan.tci));
		}
	}


	uint64_t time = timer_frequency();

#if DEBUG
	printf("Input:  [%02d] %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
			local_port,
			(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
			(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
			(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
			(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
#endif /* DEBUG */

	bool input(VNIC* vnic) {
		if(vnic->input_accept && list_index_of(vnic->input_accept, (void*)smac, NULL) < 0)
			return false;
uint16_t size = size1 + size2;

		if(vnic->input_closed - vnic->input_wait_grace > time) {
			printf("closed dropped %016lx ", vnic->mac);
			goto dropped;
		}

		uint16_t buffer_size = vnic->padding_head + size + vnic->padding_tail + (ALIGN - 1);
		if(buffer_size > vnic->max_buffer_size) {
			printf("buffer dropped %016lx ", vnic->mac);
			goto dropped;
		}

		if(buffer_size < vnic->min_buffer_size)
			buffer_size = vnic->min_buffer_size;

		if(!fifo_available(vnic->ni->input_buffer)) {
			printf("fifo dropped %016lx ", vnic->mac);
			goto dropped;
		}

		// Packet
		Packet* packet = ni_alloc(vnic->ni, buffer_size);
		if(!packet) {
			printf("memory dropped %016lx ", vnic->mac);
			goto dropped;
		}

		packet->time = time;
		packet->start = (((uint64_t)packet->buffer + vnic->padding_head + (ALIGN - 1)) & ~(ALIGN - 1)) - (uint64_t)packet->buffer;
		packet->end = packet->start + size;
		packet->size = buffer_size;

		// Copy
		if(size1 > 0)
			memcpy(packet->buffer + packet->start, buf1, size1);

		if(size2 > 0)
			memcpy(packet->buffer + packet->start + size1, buf2, size2);


		if(vnic->port & 0xfff) {
			if(!vlan_input_process(vnic, packet)) {
				ni_free(packet);
				goto dropped;
			}
		}
#if DEBUG
		printf("Input:  [%02d] %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
				local_port,
				(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
				(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
				(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
				(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
#endif /* DEBUG */

		// Push
		if(!fifo_push(vnic->ni->input_buffer, packet)) {
			ni_free(packet);
			printf("fifo dropped %016lx ", vnic->mac);
			goto dropped;
		}

		vnic->ni->input_bytes += size;
		vnic->ni->input_packets++;
		if(vnic->input_closed > time)
			vnic->input_closed += vnic->input_wait * size;
		else
			vnic->input_closed = time + vnic->input_wait * size;


		return true;

dropped:
		vnic->ni->input_drop_bytes += size;
		vnic->ni->input_drop_packets++;

		return false;
	}

	Map* vnics = map_get(((NICPriv*)nic_current->priv)->nics, (void*)(uint64_t)((local_port << 12) | vid));
	if(!vnics)
		return;

	if(dmac & ETHER_MULTICAST) {
		MapIterator iter;
		map_iterator_init(&iter, vnics);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			VNIC* vnic = entry->data;

			if(input(vnic))
				nic_rx++;
		}
	} else {
		VNIC* vnic = map_get(vnics, (void*)dmac);
		if(!vnic)
			vnic = map_get(vnics, (void*)(uint64_t)0xffffffffffff);
		
		if(!vnic)
			return;
		
		if(input(vnic))
			nic_rx++;
	}
}

Packet* nic_process_output(uint8_t local_port) {
	uint64_t time = timer_frequency();

	Map* nics = ((NICPriv*)nic_current->priv)->nics;

	MapIterator iter0;
	map_iterator_init(&iter0, nics);
	while(map_iterator_has_next(&iter0)) {
		MapEntry* entry0 = map_iterator_next(&iter0);
		uint16_t port = (uint16_t)(uint64_t)entry0->key;
		if(local_port != (port >> 12)) {
			continue;
		}

		Map* vnics = entry0->data;
		MapIterator iter1;
		map_iterator_init(&iter1, vnics);
		while(map_iterator_has_next(&iter1)) {
			MapEntry* entry1 = map_iterator_next(&iter1);
			VNIC* vnic = entry1->data;

			if(vnic->output_closed - vnic->output_wait_grace > time) {
				printf("output closed: %016lx\n", vnic->mac);
				continue;
			}

			Packet* packet = fifo_pop(vnic->ni->output_buffer);
			if(!packet) {
				continue;
			}

			if(vnic->port & 0xfff) {
				if(!vlan_output_process(vnic, packet)) {
					ni_free(packet);
					continue;
				}
			}

			uint16_t size = packet->end - packet->start;
			vnic->ni->output_bytes += size;
			vnic->ni->output_packets++;
			if(vnic->output_closed > time)
				vnic->output_closed += vnic->input_wait * size;
			else
				vnic->output_closed = time + vnic->input_wait * size;


			Ether* ether = (void*)(packet->buffer + packet->start);
			uint64_t dmac = endian48(ether->dmac);

			if(vnic->output_accept && list_index_of(vnic->output_accept, (void*)dmac, NULL) < 0) {
				/*Debuging */
				ni_free(packet);
				// Packet eliminated
				continue;
			}
			
			if(vnic->mac != 0xffffffffffff)
				ether->smac = endian48(vnic->mac);	// Fix mac address
			
#if DEBUG
			uint64_t smac = endian48(ether->smac);
			printf("Output: [%02d] %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
					local_port,
					(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
					(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
					(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
					(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
#endif /* DEBUG */

			return packet;
		}
	}

	return NULL;
}

void nic_statistics(uint64_t time) {
	/*
	   printf("\033[0;20HRX: %ld TX: %ld TX: %ld  ", nic_rx, nic_tx2, nic_tx);
	   nic_rx = nic_tx = nic_tx2 = 0;

	   MapIterator iter;
	   map_iterator_init(&iter, nis);
	   while(map_iterator_has_next(&iter)) {
	   MapEntry* entry = map_iterator_next(&iter);
	   VNIC* ni = entry->data;

	   printf("\033[1;0H%012x %d/%d %d/%d %d/%d    ", nic->mac, ni_pool_used(ni->ni), ni_pool_total(ni->ni), ni->ni->input_packets, ni->ni->output_packets, ni->ni->input_drop_packets, ni->ni->output_drop_packets);
	   }
	 */
}
