#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <tlsf.h>
#include <sys/mman.h>
#include <net/ether.h>
#include <net/ip.h>
#include <util/event.h>
#include <gmalloc.h>

#include "vnic.h"

#define DEBUG	1

#define ALIGN	16

Map* vnics;

uint64_t vnic_mac;
uint64_t vnic_rx;
uint64_t vnic_tx;
uint64_t vnic_tx2;

/*
static void polled(Device* device, void* buf, size_t size, void* data) {
	nic_process_input(buf, size, NULL, 0);
}
*/

void nic_init0() {
	vnics = map_create(4, map_uint64_hash, map_uint64_equals, NULL);
}

static void* bmalloc() {
	void* ptr;
	if((ptr = mmap(0, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
		return NULL;
	else
		return ptr;
	//return tlsf_malloc(0x200000);
}

static void bfree(void* p) {
	munmap(p, 0x200000);
	//tlsf_free(p);
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
		while(attrs[i * 2] != NIC_NONE) {
			if(attrs[i * 2] == key)
				return true;
			
			i++;
		}
		
		return false;
	}
	
	uint64_t get_value(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != NIC_NONE) {
			if(attrs[i * 2] == key)
				return attrs[i * 2 + 1];
			
			i++;
		}
		
		return (uint64_t)-1;
	}
	
	errno = 0;
	
	if(!has_key(NIC_MAC) || !has_key(NIC_POOL_SIZE) || 
			!has_key(NIC_INPUT_BANDWIDTH) || !has_key(NIC_OUTPUT_BANDWIDTH) ||
			!has_key(NIC_INPUT_BUFFER_SIZE) || !has_key(NIC_OUTPUT_BUFFER_SIZE)) {
		errno = 1;
		return NULL;
	}
	
	if((has_key(NIC_INPUT_ACCEPT_ALL) && has_key(NIC_INPUT_ACCEPT)) ||
			(has_key(NIC_OUTPUT_ACCEPT_ALL) && has_key(NIC_OUTPUT_ACCEPT))) {
		errno = 2;
		return NULL;
	}
	
	uint64_t mac = get_value(NIC_MAC);
	if(map_contains(vnics, (void*)mac)) {
		errno = 3;
		return NULL;
	}
	
	// Allocate NI
	VNIC* vnic = gmalloc(sizeof(VNIC));
	if(!vnic)
		return NULL;
	bzero(vnic, sizeof(VNIC));
	
	// Allocate initial pool
	uint64_t pool_size = get_value(NIC_POOL_SIZE);
	if(pool_size == 0 || (pool_size & ~(0x200000 - 1)) != pool_size) {
		tlsf_free(vnic);
		errno = 4;
		return NULL;
	}
	
	//set shared blloac
	void* pool = bmalloc();
	if(pool == NULL) {
		tlsf_free(vnic);
		errno = 5;
		return NULL;
	}
	
	init_memory_pool(0x200000, pool, 1);
	vnic->pool = pool;
	
	// Allocate extra pools
	vnic->pools = list_create(vnic->pool);
	if(!vnic->pools) {
		tlsf_free(vnic);
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
			tlsf_free(vnic);
			errno = 6;
			
			return NULL;
		}
		
		list_add(vnic->pools, ptr);
		add_new_area(ptr, 0x200000, pool);
	}
	
	// Allocate NetworkInterface
	vnic->nic = malloc_ex(sizeof(NIC), vnic->pool);
	bzero(vnic->nic, sizeof(NIC));
	
	// Default attributes
	vnic->padding_head = 32;
	vnic->padding_tail = 32;
	vnic->min_buffer_size = 128;
	vnic->max_buffer_size = 2048;
	if(!has_key(NIC_INPUT_ACCEPT_ALL))
		vnic->input_accept = list_create(vnic->pool);
	if(!has_key(NIC_OUTPUT_ACCEPT_ALL))
		vnic->output_accept = list_create(vnic->pool);
	
	//ni->input_closed = ni->output_closed = cpu_tsc();
	
	// Extra attributes
	int i = 0;
	while(attrs[i * 2] != NIC_NONE) {
		switch(attrs[i * 2]) {
			case NIC_MAC:
				vnic->nic->mac = vnic->mac = attrs[i * 2 + 1];
				break;
			case NIC_DEV:
				vnic->port = (int)attrs[i * 2 + 1];
				break;
			case NIC_POOL_SIZE:
				break;
			/*
			case NIC_INPUT_BANDWIDTH:
				vnic->nic->input_bandwidth = vnic->input_bandwidth = attrs[i * 2 + 1];
				vnic->input_wait = cpu_frequency * 8 / vnic->input_bandwidth;
				vnic->input_wait_grace = cpu_frequency / 100;
				break;
			case NIC_OUTPUT_BANDWIDTH:
				vnic->nic->output_bandwidth = vnic->output_bandwidth = attrs[i * 2 + 1];
				vnic->output_wait = cpu_frequency * 8 / vnic->output_bandwidth;
				vnic->output_wait_grace = cpu_frequency / 1000;
				break;
			*/
			case NIC_PADDING_HEAD:
				vnic->nic->padding_head = vnic->padding_head = attrs[i * 2 + 1];
				break;
			case NIC_PADDING_TAIL:
				vnic->nic->padding_tail = vnic->padding_tail = attrs[i * 2 + 1];
				break;
			case NIC_INPUT_BUFFER_SIZE:
				vnic->nic->input_buffer = fifo_create(attrs[i * 2 + 1], vnic->pool);
				if(!vnic->nic->input_buffer) {
					errno = 5;
					tlsf_free(vnic);
					return NULL;
				}
				break;
			case NIC_OUTPUT_BUFFER_SIZE:
				vnic->nic->output_buffer = fifo_create(attrs[i * 2 + 1], vnic->pool);
				if(!vnic->nic->output_buffer) {
					errno = 5;
					tlsf_free(vnic);
					return NULL;
				}
				break;
			case NIC_MIN_BUFFER_SIZE:
				vnic->nic->min_buffer_size = vnic->min_buffer_size = attrs[i * 2 + 1];
				break;
			case NIC_MAX_BUFFER_SIZE:
				vnic->nic->max_buffer_size = vnic->max_buffer_size = attrs[i * 2 + 1];
				break;
			case NIC_INPUT_ACCEPT:
				if(vnic->input_accept) {
					if(!list_add(vnic->input_accept, (void*)attrs[i * 2 + 1])) {
						errno = 5;
						tlsf_free(vnic);
						return NULL;
					}
				}
				break;
			case NIC_OUTPUT_ACCEPT:
				if(vnic->output_accept) {
					if(!list_add(vnic->output_accept, (void*)attrs[i * 2 + 1])) {
						errno = 5;
						tlsf_free(vnic);
						return NULL;
					}
				}
				break;
		}
		
		i++;
	}
	vnic->nic->pool = vnic->pool;
	
	// Config
	vnic->nic->config = map_create(8, NULL, NULL, vnic->pool);
	
	// Register the ni
	map_put(vnics, (void*)vnic->mac, vnic);
	
	return vnic;
}

void vnic_destroy(VNIC* vnic) {
	// Unregister the ni
	map_remove(vnics, (void*)vnic->mac);
	
	// Free pools
	ListIterator iter;
	list_iterator_init(&iter, vnic->pools);
	
	while(list_iterator_has_next(&iter)) {
		void* pool = list_iterator_next(&iter);
		bfree(pool);
	}
	
	tlsf_free(vnic);
}

bool vnic_contains(char* nic_dev, uint64_t mac) {
	return map_contains(vnics, (void*)mac);
}

uint32_t vnic_update(VNIC* vnic, uint64_t* attrs) {
	uint32_t result = 0;
	
	List* pools = NULL;
	void* input_buffer = NULL;
	uint64_t input_buffer_size = 0;
	void* output_buffer = NULL;
	uint64_t output_buffer_size = 0;
	List* input_accept = NULL;
	List* output_accept = NULL;
	
	int i = 0;
	while(attrs[i * 2] != NIC_NONE) {
		switch(attrs[i * 2]) {
			case NIC_MAC:
				result = 1;
				goto failed;
			case NIC_POOL_SIZE:
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
			case NIC_INPUT_BANDWIDTH:
				break;
			case NIC_OUTPUT_BANDWIDTH:
				break;
			case NIC_PADDING_HEAD:
				break;
			case NIC_PADDING_TAIL:
				break;
			case NIC_INPUT_BUFFER_SIZE:
				input_buffer_size = attrs[i * 2 + 1];
				input_buffer = malloc_ex(sizeof(void*) * input_buffer_size, vnic->pool);
				if(!input_buffer) {
					result = 5;
					goto failed;
				}
				break;
			case NIC_OUTPUT_BUFFER_SIZE:
				output_buffer_size = attrs[i * 2 + 1];
				output_buffer = malloc_ex(sizeof(void*) * output_buffer_size, vnic->pool);
				if(!output_buffer) {
					result = 5;
					goto failed;
				}
				break;
			case NIC_MIN_BUFFER_SIZE:
				break;
			case NIC_MAX_BUFFER_SIZE:
				break;
			case NIC_INPUT_ACCEPT:
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
			case NIC_OUTPUT_ACCEPT:
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
		nic_free(input_buffer);
	
	if(output_buffer)
		nic_free(output_buffer);
	
	if(input_accept)
		list_destroy(input_accept);
	
	if(output_accept)
		list_destroy(output_accept);
	
	return result;
	
succeed:
	
	// NIC_POOL_SIZE
	if(pools) {
		ListIterator iter;
		
		list_iterator_init(&iter, pools);
		while(list_iterator_has_next(&iter)) {
			void* ptr = list_iterator_remove(&iter);
			add_new_area(ptr, 0x200000, vnic->pool);
		}
		list_destroy(pools);
		vnic->nic->pool_size = list_size(vnic->pools) * 0x200000;
	}
	
	// NIC_INPUT_BUFFER_SIZE
	if(input_buffer) {
		void input_popped(void* data) {
			Packet* packet = data;
			
			vnic->nic->input_drop_bytes += packet->end - packet->start;
			vnic->nic->input_drop_packets++;
			
			nic_free(packet);
		}
		
		void* array = vnic->nic->input_buffer->array;
		fifo_reinit(vnic->nic->input_buffer, input_buffer, input_buffer_size, input_popped);
		free_ex(array, vnic->pool);
	}
	
	// NIC_OUTPUT_BUFFER_SIZE
	if(output_buffer) {
		void output_popped(void* data) {
			Packet* packet = data;
			
			vnic->nic->output_drop_bytes += packet->end - packet->start;
			vnic->nic->output_drop_packets++;
			
			free_ex(packet->buffer, vnic->pool);
			free_ex(packet, vnic->pool);
		}
		
		void* array = vnic->nic->output_buffer->array;
		fifo_reinit(vnic->nic->output_buffer, output_buffer, output_buffer_size, output_popped);
		free_ex(array, vnic->pool);
	}
	
	// NIC_INPUT_ACCEPT
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
	
	// NIC_OUTPUT_ACCEPT
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
	while(attrs[i * 2] != NIC_NONE) {
		switch(attrs[i * 2]) {
			/*
			case NIC_INPUT_BANDWIDTH:
				vnic->nic->input_bandwidth = vnic->input_bandwidth = attrs[i * 2 + 1];
				vnic->input_wait = cpu_frequency * 8 / vnic->input_bandwidth;
				vnic->input_wait_grace = cpu_frequency / 100;
				break;
			case NIC_OUTPUT_BANDWIDTH:
				vnic->nic->output_bandwidth = vnic->output_bandwidth = attrs[i * 2 + 1];
				vnic->output_wait = cpu_frequency * 8 / vnic->output_bandwidth;
				vnic->output_wait_grace = cpu_frequency / 1000;
				break;
			*/
			case NIC_PADDING_HEAD:
				vnic->padding_head = attrs[i * 2 + 1];
				break;
			case NIC_PADDING_TAIL:
				vnic->padding_tail = attrs[i * 2 + 1];
				break;
			case NIC_MIN_BUFFER_SIZE:
				vnic->min_buffer_size = attrs[i * 2 + 1];
				break;
			case NIC_MAX_BUFFER_SIZE:
				vnic->max_buffer_size = attrs[i * 2 + 1];
				break;
			case NIC_INPUT_ACCEPT_ALL:
				if(vnic->input_accept) {
					list_destroy(vnic->input_accept);
					vnic->input_accept = NULL;
				}
				break;
			case NIC_OUTPUT_ACCEPT_ALL:
				if(vnic->output_accept) {
					list_destroy(vnic->output_accept);
					vnic->output_accept = NULL;
				}
				break;
		}
	}
	
	return 0;
}

void nic_process_input(uint8_t local_port, uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2) {
	// Get dmac, smac
	uint64_t dmac;
	uint64_t smac;
	
	if(size1 >= 12) {
		Ether* ether = (Ether*)buf1;
		dmac = endian48(ether->dmac);
		smac = endian48(ether->smac);
	} else {
		Ether ether;
		uint8_t* p = (uint8_t*)&ether;
		memcpy(p, buf1, size1);
		memcpy(p + size1, buf2, 12 - size1);
		dmac = endian48(ether.dmac);
		smac = endian48(ether.smac);
	}
	
	//uint64_t time = cpu_tsc();
	
	#if DEBUG
	printf("Input:  [%02d] %02lx:%02lx:%02lx:%02lx:%02lx:%02lx %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n", 
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
		
		/*
		if(vnic->input_closed - vnic->input_wait_grace > time) {
			printf("closed dropped %016lx ", vnic->mac);
			goto dropped;
		}
		*/
		
		uint16_t buffer_size = vnic->padding_head + size + vnic->padding_tail + (ALIGN - 1);
		if(buffer_size > vnic->max_buffer_size) {
			printf("buffer dropped %016lx ", vnic->mac);
			goto dropped;
		}
		
		if(buffer_size < vnic->min_buffer_size)
			buffer_size = vnic->min_buffer_size;
		
		if(!fifo_available(vnic->nic->input_buffer)) {
			printf("fifo dropped %016lx ", vnic->mac);
			goto dropped;
		}
		
		// Packet
		Packet* packet = nic_alloc(vnic->nic, buffer_size);
		if(!packet) {
			printf("memory dropped %016lx ", vnic->mac);
			goto dropped;
		}
		
		//packet->time = time;
		packet->start = (((uint64_t)packet->buffer + vnic->padding_head + (ALIGN - 1)) & ~(ALIGN - 1)) - (uint64_t)packet->buffer;
		packet->end = packet->start + size;
		packet->size = buffer_size;
		
		// Copy
		if(size1 > 0)
			memcpy(packet->buffer + packet->start, buf1, size1);
		
		if(size2 > 0)
			memcpy(packet->buffer + packet->start + size1, buf2, size2);
		
		
		if(!fifo_push(vnic->nic->input_buffer, packet)) {
			nic_free(packet);
			printf("fifo dropped %016lx ", vnic->mac);
			goto dropped;
		}
		
		vnic->nic->input_bytes += size;
		vnic->nic->input_packets++;
		/*
		if(ni->input_closed > time)
			ni->input_closed += ni->input_wait * size;
		else
			ni->input_closed = time + ni->input_wait * size;
		*/
		
		return true;

dropped:
		vnic->nic->input_drop_bytes += size;
		vnic->nic->input_drop_packets++;
		
		return false;
	}
	
	if(dmac & ETHER_MULTICAST) {
		MapIterator iter;
		map_iterator_init(&iter, vnics);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			VNIC* vnic = entry->data;
			if(vnic->port == local_port) {
				if(input(vnic)) {
					vnic_rx++;
				}
			}
		}
	} else {
		VNIC* vnic = map_get(vnics, (void*)dmac);
		if(vnic && vnic->port == local_port) {
			if(input(vnic))
				vnic_rx++;
		}
	}
}

Packet* nic_process_output(uint8_t local_port) {
	//uint64_t time = cpu_tsc();
	
	MapIterator iter;
	map_iterator_init(&iter, vnics);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		VNIC* vnic = entry->data;
		if(vnic->port != local_port)
			continue;
		
		/*
		if(vnic->output_closed - ni->output_wait_grace > time) {
			printf("output closed: %016lx\n", vnic->mac);
			continue;
		}
		*/
		
		Packet* packet = fifo_pop(vnic->nic->output_buffer);
		if(!packet)
			continue;
		
		uint16_t size = packet->end - packet->start;
		vnic->nic->output_bytes += size;
		vnic->nic->output_packets++;
		/*
		if(vnic->output_closed > time)
			vnic->output_closed += vnic->input_wait * size;
		else
			vnic->output_closed = time + vnic->input_wait * size;
		*/
		
		
		Ether* ether = (void*)(packet->buffer + packet->start);
		uint64_t dmac = endian48(ether->dmac);
		
		if(vnic->output_accept && list_index_of(vnic->output_accept, (void*)dmac, NULL) < 0) {
			nic_free(packet);
			// Packet eliminated
			continue;
		}
		
		ether->smac = endian48(vnic->mac);	// Fix mac address
		
		#if DEBUG
		uint64_t smac = vnic->mac;
		printf("Output: [%02d] %02lx:%02lx:%02lx:%02lx:%02lx:%02lx %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n", 
			local_port,
			(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
			(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
			(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
			(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
		#endif /* DEBUG */
		
		return packet;
	}
	
	return NULL;
}

void vnic_statistics(uint64_t time) {
	/*
	printf("\033[0;20HRX: %ld TX: %ld TX: %ld  ", nic_rx, nic_tx2, nic_tx);
	nic_rx = nic_tx = nic_tx2 = 0;
	
	MapIterator iter;
	map_iterator_init(&iter, nis);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		NI* ni = entry->data;
		
		printf("\033[1;0H%012x %d/%d %d/%d %d/%d    ", ni->mac, nic_pool_used(ni->nic), nic_pool_total(ni->nic), ni->nic->input_packets, ni->nic->output_packets, ni->nic->input_drop_packets, ni->nic->output_drop_packets);
	}
	*/
}
