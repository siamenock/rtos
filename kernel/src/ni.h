#ifndef __NI_H__
#define __NI_H__

#include <stdint.h>
#include <stdbool.h>
#include <util/fifo.h>
#include <util/list.h>
#include <net/ni.h>

typedef struct {
	// Management
	NetworkInterface*	ni;
	List*		pools;
	void*		pool;
	
	// Information
	uint64_t	mac;
	uint64_t	input_bandwidth;
	uint64_t	input_wait;
	uint64_t	input_wait_grace;
	uint64_t	output_bandwidth;
	uint64_t	output_wait;
	uint64_t	output_wait_grace;
	uint16_t	padding_head;
	uint16_t	padding_tail;
	uint16_t	min_buffer_size;
	uint16_t	max_buffer_size;
	List*		input_accept;
	List*		output_accept;
	
	// Buffer
	uint64_t	input_closed;
	uint64_t	output_closed;
} NI;

extern uint64_t ni_mac;

void ni_init0();

/**
 * Mandatory attributes to create a NI
 * mac, input_bandwidth, output_bandwidth, input_buffer_size, output_buffer_size, pool_size
 */
NI* ni_create(uint64_t* attrs);
void ni_destroy(NI* ni);
bool ni_contains(uint64_t mac);

/**
 * Return
 * 0: succeed
 * 1: MAC address cannot changable
 * 2: pool size cannot be reduced.
 * 3: pool size must be multiple of 2MBs
 * 4: pool size cannot reduced
 * 5: not enough space
 * 6: no resource to allocate
 */
uint32_t ni_update(NI* ni, uint64_t* attrs, uint32_t size);
void ni_process_input(uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2);
Packet* ni_process_output();
void ni_statistics(uint64_t time);

#endif /* __NI_H__ */
