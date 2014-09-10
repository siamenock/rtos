#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <tlsf.h>
#include <net/ether.h>
#include <net/ip.h>
#include <util/map.h>
#include <util/event.h>
#include "gmalloc.h"
#include "mp.h"
#include "cpu.h"
#include "pci.h"
#include "device.h"
#include "../../loader/src/page.h"	// TODO: Delete it

#include "ni.h"

#define DEBUG	1

#define ALIGN	16

Map* nis;

uint64_t ni_mac;
uint64_t ni_rx;
uint64_t ni_tx;
uint64_t ni_tx2;

/*
static void polled(Device* device, void* buf, size_t size, void* data) {
	ni_process_input(buf, size, NULL, 0);
}
*/

void ni_init0() {
	nis = map_create(4, map_uint64_hash, map_uint64_equals, NULL);
}

/**
 *
 * Error: errno
 * 1: mandatory attributes not specified
 * 2: Conflict attributes speicifed
 * 3: Pool size is not multiple of 2MB
 * 4: Not enough memory
 */
NI* ni_create(uint64_t* attrs) {
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
	
	if(!has_key(NI_MAC) || !has_key(NI_POOL_SIZE) || 
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
	
	uint64_t mac = get_value(NI_MAC);
	if(map_contains(nis, (void*)mac)) {
		errno = 3;
		return NULL;
	}
	 
	// Allocate NI
	NI* ni = gmalloc(sizeof(NI));
	if(!ni)
		return NULL;
	bzero(ni, sizeof(NI));
	
	// Allocate initial pool
	uint64_t pool_size = get_value(NI_POOL_SIZE);
	if(pool_size == 0 || (pool_size & ~(0x200000 - 1)) != pool_size) {
		gfree(ni);
		errno = 4;
		return NULL;
	}
	
	void* pool = bmalloc();
	if(pool == NULL) {
		gfree(ni);
		errno = 5;
		return NULL;
	}
	
	init_memory_pool(0x200000, pool, 1);
	ni->pool = pool;
	
	// Allocate extra pools
	ni->pools = list_create(ni->pool);
	if(!ni->pools) {
		gfree(ni);
		errno = 5;
		return NULL;
	}
	list_add(ni->pools, pool);
	uint32_t pool_count = pool_size / 0x200000 - 1;
 	for(uint32_t i = 0; i < pool_count; i++) {
		void* ptr = bmalloc();
		
		if(!ptr) {
			while(!list_is_empty(ni->pools)) {
				ptr = list_remove_first(ni->pools);
				bfree(ptr);
			}
			gfree(ni);
			errno = 6;
			
			return NULL;
		}
		
		list_add(ni->pools, ptr);
		add_new_area(ptr, 0x200000, pool);
	}
	
	// Allocate NetworkInterface
	ni->ni = malloc_ex(sizeof(NetworkInterface), ni->pool);
	bzero(ni->ni, sizeof(NetworkInterface));
	
	// Default attributes
	ni->padding_head = 32;
	ni->padding_tail = 32;
	ni->min_buffer_size = 128;
	ni->max_buffer_size = 2048;
	if(!has_key(NI_INPUT_ACCEPT_ALL))
		ni->input_accept = list_create(ni->pool);
	if(!has_key(NI_OUTPUT_ACCEPT_ALL))
		ni->output_accept = list_create(ni->pool);
	
	ni->input_closed = ni->output_closed = cpu_tsc();
	
	// Extra attributes
	int i = 0;
	while(attrs[i * 2] != NI_NONE) {
		switch(attrs[i * 2]) {
			case NI_MAC:
				ni->ni->mac = ni->mac = attrs[i * 2 + 1];
				break;
			case NI_POOL_SIZE:
				break;
			case NI_INPUT_BANDWIDTH:
				ni->ni->input_bandwidth = ni->input_bandwidth = attrs[i * 2 + 1];
				ni->input_wait = cpu_frequency * 8 / ni->input_bandwidth;
				ni->input_wait_grace = cpu_frequency / 100;
				break;
			case NI_OUTPUT_BANDWIDTH:
				ni->ni->output_bandwidth = ni->output_bandwidth = attrs[i * 2 + 1];
				ni->output_wait = cpu_frequency * 8 / ni->output_bandwidth;
				ni->output_wait_grace = cpu_frequency / 1000;
				break;
			case NI_PADDING_HEAD:
				ni->ni->padding_head = ni->padding_head = attrs[i * 2 + 1];
				break;
			case NI_PADDING_TAIL:
				ni->ni->padding_tail = ni->padding_tail = attrs[i * 2 + 1];
				break;
			case NI_INPUT_BUFFER_SIZE:
				ni->ni->input_buffer = fifo_create(attrs[i * 2 + 1], ni->pool);
				if(!ni->ni->input_buffer) {
					errno = 5;
					gfree(ni);
					return NULL;
				}
				break;
			case NI_OUTPUT_BUFFER_SIZE:
				ni->ni->output_buffer = fifo_create(attrs[i * 2 + 1], ni->pool);
				if(!ni->ni->output_buffer) {
					errno = 5;
					gfree(ni);
					return NULL;
				}
				break;
			case NI_MIN_BUFFER_SIZE:
				ni->ni->min_buffer_size = ni->min_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_MAX_BUFFER_SIZE:
				ni->ni->max_buffer_size = ni->max_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_INPUT_ACCEPT:
				if(ni->input_accept) {
					if(!list_add(ni->input_accept, (void*)attrs[i * 2 + 1])) {
						errno = 5;
						gfree(ni);
						return NULL;
					}
				}
				break;
			case NI_OUTPUT_ACCEPT:
				if(ni->output_accept) {
					if(!list_add(ni->output_accept, (void*)attrs[i * 2 + 1])) {
						errno = 5;
						gfree(ni);
						return NULL;
					}
				}
				break;
		}
		
		i++;
	}
	ni->ni->pool = ni->pool;
	
	// Config
	ni->ni->config = map_create(8, NULL, NULL, ni->pool);
	
	// Register the ni
	map_put(nis, (void*)ni->mac, ni);
	
	return ni;
}

void ni_destroy(NI* ni) {
	// Unregister the ni
	map_remove(nis, (void*)ni->mac);
	
	// Free pools
	ListIterator iter;
	list_iterator_init(&iter, ni->pools);
	
	while(list_iterator_has_next(&iter)) {
		void* pool = list_iterator_next(&iter);
		bfree(pool);
	}
	
	gfree(ni);
}

bool ni_contains(uint64_t mac) {
	return map_contains(nis, (void*)mac);
}

uint32_t ni_update(NI* ni, uint64_t* attrs, uint32_t size) {
	uint32_t result = 0;
	
	List* pools = NULL;
	void* input_buffer = NULL;
	uint64_t input_buffer_size = 0;
	void* output_buffer = NULL;
	uint64_t output_buffer_size = 0;
	List* input_accept = NULL;
	List* output_accept = NULL;
	
	for(uint32_t i = 0; i < size; i++) {
		switch(attrs[i * 2]) {
			case NI_MAC:
				result = 1;
				goto failed;
			case NI_POOL_SIZE:
				;
				uint64_t pool_size = attrs[i * 2 + 1];
				if(pool_size < list_size(ni->pools) * 0x200000) {
					result = 2;
					goto failed;
				}
				
				if((pool_size & ~(0x200000 - 1)) != pool_size) {
					result = 3;
					goto failed;
				}
				
				int32_t pools_count = pool_size / 0x200000 - list_size(ni->pools);
				if(pools_count < 0) {
					result = 4;
					goto failed;
				}
				
				pools = list_create(ni->pool);
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
				input_buffer = malloc_ex(sizeof(void*) * input_buffer_size, ni->pool);
				if(!input_buffer) {
					result = 5;
					goto failed;
				}
				break;
			case NI_OUTPUT_BUFFER_SIZE:
				output_buffer_size = attrs[i * 2 + 1];
				output_buffer = malloc_ex(sizeof(void*) * output_buffer_size, ni->pool);
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
					input_accept = list_create(ni->pool);
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
					output_accept = list_create(ni->pool);
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
			add_new_area(ptr, 0x200000, ni->pool);
		}
		list_destroy(pools);
		ni->ni->pool_size = list_size(ni->pools) * 0x200000;
	}
	
	// NI_INPUT_BUFFER_SIZE
	if(input_buffer) {
		void input_popped(void* data) {
			Packet* packet = data;
			
			ni->ni->input_drop_bytes += packet->end - packet->start;
			ni->ni->input_drop_packets++;
			
			ni_free(packet);
		}
		
		void* array = ni->ni->input_buffer->array;
		fifo_reinit(ni->ni->input_buffer, input_buffer, input_buffer_size, input_popped);
		free_ex(array, ni->pool);
	}
	
	// NI_OUTPUT_BUFFER_SIZE
	if(output_buffer) {
		void output_popped(void* data) {
			Packet* packet = data;
			
			ni->ni->output_drop_bytes += packet->end - packet->start;
			ni->ni->output_drop_packets++;
			
			free_ex(packet->buffer, ni->pool);
			free_ex(packet, ni->pool);
		}
		
		void* array = ni->ni->output_buffer->array;
		fifo_reinit(ni->ni->output_buffer, output_buffer, output_buffer_size, output_popped);
		free_ex(array, ni->pool);
	}
	
	// NI_INPUT_ACCEPT
	if(input_accept) {
		if(!ni->input_accept) {
			ni->input_accept = input_accept;
		} else {
			while(list_size(input_accept) > 0) {
				list_add(ni->input_accept, list_remove_first(input_accept));
			}
			list_destroy(input_accept);
		}
	}
	
	// NI_OUTPUT_ACCEPT
	if(output_accept) {
		if(!ni->output_accept) {
			ni->output_accept = output_accept;
		} else {
			while(list_size(output_accept) > 0) {
				list_add(ni->output_accept, list_remove_first(output_accept));
			}
			list_destroy(output_accept);
		}
	}
	
	for(uint32_t i = 0; i < size; i++) {
		switch(attrs[i * 2]) {
			case NI_INPUT_BANDWIDTH:
				ni->ni->input_bandwidth = ni->input_bandwidth = attrs[i * 2 + 1];
				ni->input_wait = cpu_frequency * 8 / ni->input_bandwidth;
				ni->input_wait_grace = cpu_frequency / 100;
				break;
			case NI_OUTPUT_BANDWIDTH:
				ni->ni->output_bandwidth = ni->output_bandwidth = attrs[i * 2 + 1];
				ni->output_wait = cpu_frequency * 8 / ni->output_bandwidth;
				ni->output_wait_grace = cpu_frequency / 1000;
				break;
			case NI_PADDING_HEAD:
				ni->padding_head = attrs[i * 2 + 1];
				break;
			case NI_PADDING_TAIL:
				ni->padding_tail = attrs[i * 2 + 1];
				break;
			case NI_MIN_BUFFER_SIZE:
				ni->min_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_MAX_BUFFER_SIZE:
				ni->max_buffer_size = attrs[i * 2 + 1];
				break;
			case NI_INPUT_ACCEPT_ALL:
				if(ni->input_accept) {
					list_destroy(ni->input_accept);
					ni->input_accept = NULL;
				}
				break;
			case NI_OUTPUT_ACCEPT_ALL:
				if(ni->output_accept) {
					list_destroy(ni->output_accept);
					ni->output_accept = NULL;
				}
				break;
		}
	}
	
	return 0;
}

void ni_process_input(uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2) {
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
	
	uint64_t time = cpu_tsc();
	
	#if DEBUG
	printf("Input:  %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n", 
		(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
		(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
		(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
		(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
	#endif /* DEBUG */
	
	bool input(NI* ni) {
		if(ni->input_accept && list_index_of(ni->input_accept, (void*)smac, NULL) < 0)
			return false;
		
		uint16_t size = size1 + size2;
		
		if(ni->input_closed - ni->input_wait_grace > time) {
			printf("closed dropped %016lx ", ni->mac);
			goto dropped;
		}
		
		uint16_t buffer_size = ni->padding_head + size + ni->padding_tail + (ALIGN - 1);
		if(buffer_size > ni->max_buffer_size) {
			printf("buffer dropped %016lx ", ni->mac);
			goto dropped;
		}
		
		if(buffer_size < ni->min_buffer_size)
			buffer_size = ni->min_buffer_size;
		
		if(!fifo_available(ni->ni->input_buffer)) {
			printf("fifo dropped %016lx ", ni->mac);
			goto dropped;
		}
		
		// Packet
		Packet* packet = ni_alloc(ni->ni, buffer_size);
		if(!packet) {
			printf("memory dropped %016lx ", ni->mac);
			goto dropped;
		}
		
		packet->time = time;
		packet->start = (((uint64_t)packet->buffer + ni->padding_head + (ALIGN - 1)) & ~(ALIGN - 1)) - (uint64_t)packet->buffer;
		packet->end = packet->start + size;
		packet->size = buffer_size;
		
		// Copy
		if(size1 > 0)
			memcpy(packet->buffer + packet->start, buf1, size1);
		
		if(size2 > 0)
			memcpy(packet->buffer + packet->start + size1, buf2, size2);
		
		
		// Push
		if(!fifo_push(ni->ni->input_buffer, packet)) {
			ni_free(packet);
			printf("fifo dropped %016lx ", ni->mac);
			goto dropped;
		}
		
		ni->ni->input_bytes += size;
		ni->ni->input_packets++;
		if(ni->input_closed > time)
			ni->input_closed += ni->input_wait * size;
		else
			ni->input_closed = time + ni->input_wait * size;
		
		return true;

dropped:
		ni->ni->input_drop_bytes += size;
		ni->ni->input_drop_packets++;
		
		return false;
	}
	
	if(dmac & ETHER_MULTICAST) {
		MapIterator iter;
		map_iterator_init(&iter, nis);
		bool result = false;
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			NI* ni = entry->data;
			result |= input(ni);
		}
		if(result)
			ni_rx++;
	} else {
		NI* ni = map_get(nis, (void*)dmac);
		if(ni) {
			if(input(ni))
				ni_rx++;
		}
	}
}

Packet* ni_process_output() {
	uint64_t time = cpu_tsc();
	
	MapIterator iter;
	map_iterator_init(&iter, nis);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		NI* ni = entry->data;
		if(ni->output_closed - ni->output_wait_grace > time) {
			printf("output closed: %016lx\n", ni->mac);
			continue;
		}
		
		Packet* packet = fifo_pop(ni->ni->output_buffer);
		if(!packet)
			continue;
		
		uint16_t size = packet->end - packet->start;
		ni->ni->output_bytes += size;
		ni->ni->output_packets++;
		if(ni->output_closed > time)
			ni->output_closed += ni->input_wait * size;
		else
			ni->output_closed = time + ni->input_wait * size;
		
		
		Ether* ether = (void*)(packet->buffer + packet->start);
		uint64_t dmac = endian48(ether->dmac);
		
		if(ni->output_accept && list_index_of(ni->output_accept, (void*)dmac, NULL) < 0) {
			ni_free(packet);
			// Packet eliminated
			continue;
		}
		
		ether->smac = endian48(ni->mac);	// Fix mac address
		
		#if DEBUG
		uint64_t smac = ni->mac;
		printf("Output: %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n", 
			(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
			(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
			(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
			(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
		#endif /* DEBUG */
		
		return packet;
	}
	
	return NULL;
}

void ni_statistics(uint64_t time) {
	/*
	printf("\033[0;20HRX: %ld TX: %ld TX: %ld  ", ni_rx, ni_tx2, ni_tx);
	ni_rx = ni_tx = ni_tx2 = 0;
	
	MapIterator iter;
	map_iterator_init(&iter, nis);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		NI* ni = entry->data;
		
		printf("\033[1;0H%012x %d/%d %d/%d %d/%d    ", ni->mac, ni_pool_used(ni->ni), ni_pool_total(ni->ni), ni->ni->input_packets, ni->ni->output_packets, ni->ni->input_drop_packets, ni->ni->output_drop_packets);
	}
	*/
}
