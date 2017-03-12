#ifndef __VNIC_H__
#define __VNIC_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define VNIC_MAX_COUNT		64
#define VNIC_MAX_LINKS		8
#define VNIC_CHUNK_SIZE		64
#define VNIC_MAX_SIZE		(16 * 1024 * 1024)	// 16MB
#define VNIC_HEADER_SIZE	(64 * 1024)		// 64KB

#define VNIC_MAGIC_HEADER	0x0A38E56586468C01LL	// PacketNgin vNIC 01(version)

/**
 * @file
 * Virtual Network Interface Controller (vNIC) host API
 */

// Host API
typedef struct _Packet {
	uint64_t	time;
	
	uint16_t	start;
	uint16_t	end;
	uint16_t	size;
	
	uint8_t		buffer[0];
} Packet;

typedef struct _VNIC_Queue {
	uint32_t	base;
	uint32_t	head;
	uint32_t	tail;
	uint32_t	size;
	volatile uint8_t rlock;
	volatile uint8_t wlock;
} VNIC_Queue;

typedef struct _VNIC_Pool {
	uint32_t	bitmap;
	uint32_t	count;
	uint32_t	pool;
	uint32_t	index;
	uint32_t	used;
	volatile uint8_t lock;
} VNIC_Pool;

/**
 * VNIC Memory Map
 *
 * VNIC_MAGIC_HEADER (8 bytes)
 * Metadata (bandwidth, pdding, queue, pool)
 * Config
 * Fast path input queue
 * Fast path output queue
 * Slow path input queue
 * Slow path output queue
 * Packet pool bitmap
 * Packet payload pool
 */
typedef struct _VNIC {
	// 2MBs aligned
	uint64_t	magic;

	uint32_t	id;	// VNIC unique ID (unique ID in RTOS)
	
	uint64_t	mac;
	
	uint64_t	input_bandwidth;
	uint64_t	output_bandwidth;
	
	uint16_t	padding_head;
	uint16_t	padding_tail;
	
	VNIC_Queue	input;
	VNIC_Queue	output;
	
	VNIC_Queue	slow_input;
	VNIC_Queue	slow_output;
	
	VNIC_Pool	pool;
	
	uint32_t	config;
	uint8_t		config_head[0];
	uint8_t		config_tail[0] __attribute__((__aligned__(VNIC_HEADER_SIZE)));
	
	// input queue (8 bytes aligned)
	// output queue (8 bytes aligned)
	// slow_input queue (8 bytes aligned)
	// slow_output queue (8 bytes aligned)
	// pool bitmap (8 bytes aligned)
	// pool (VNIC_CHUNK_SIZE(64) bytges aligned)
} VNIC;

int vnic_count();
VNIC* vnic_get(int index);
VNIC* vnic_get_by_id(uint32_t id);

Packet* vnic_alloc(VNIC* vnic, uint16_t size);
bool vnic_free(Packet* packet);

bool vnic_has_input(VNIC* vnic);
Packet* vnic_input(VNIC* vnic);
uint32_t vnic_input_size(VNIC* vnic);

bool vnic_has_slow_input(VNIC* vnic);
Packet* vnic_slow_input(VNIC* vnic);
uint32_t vnic_slow_input_size(VNIC* vnic);

bool vnic_output(VNIC* vnic, Packet* packet);
bool vnic_output_try(VNIC* vnic, Packet* packet);
bool vnic_output_dup(VNIC* vnic, Packet* packet);
bool vnic_output_available(VNIC* vnic);
uint32_t vnic_output_size(VNIC* vnic);

bool vnic_slow_output(VNIC* vnic, Packet* packet);
bool vnic_slow_output_try(VNIC* vnic, Packet* packet);
bool vnic_slow_output_dup(VNIC* vnic, Packet* packet);
bool vnic_slow_output_available(VNIC* vnic);
uint32_t vnic_slow_output_size(VNIC* vnic);

size_t vnic_pool_used(VNIC* vnic);
size_t vnic_pool_free(VNIC* vnic);
size_t vnic_pool_total(VNIC* vnic);

uint32_t vnic_config_register(VNIC* vnic, char* name);
uint32_t vnic_config_key(VNIC* vnic, char* name);
bool vnic_config_put(VNIC* vnic, uint32_t key, uint64_t value);
uint64_t vnic_config_get(VNIC* vnic, uint32_t key);

// Driver API

/**
 * Initialize VNIC memory map
 *
 * VNIC metadata (magic, mac, ...)
 * Input Queue
 * Output Queue
 * Slow path input queue
 * Slow path output queue
 * Malloc bitmap
 * Malloc pool
 *
 * @param base 2MBs aligned
 * @param size multiples of 2MBs
 */
void vnic_init(uint32_t id, uint64_t mac, void* base, size_t size, 
		uint64_t input_bandwidth, uint64_t output_bandwidth, 
		uint16_t padding_head, uint16_t padding_tail, 
		uint32_t input_queue_size, uint32_t output_queue_size, 
		uint32_t slow_input_queue_size, uint32_t slow_output_queue_size);

bool vnic_receivable(VNIC* vnic);
bool vnic_received(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2);
bool vnic_received2(VNIC* vnic, Packet* packet);

bool vnic_slow_receivable(VNIC* vnic);
bool vnic_slow_received(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2);
bool vnic_slow_received2(VNIC* vnic, Packet* packet);

bool vnic_sendable(VNIC* vnic);
Packet* vnic_send(VNIC* vnic);

bool vnic_slow_sendable(VNIC* vnic);
Packet* vnic_slow_send(VNIC* vnic);

#endif /* __VNIC_H__ */
