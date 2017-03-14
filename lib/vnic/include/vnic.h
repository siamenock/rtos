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
 * Fast path rx queue
 * Fast path tx queue
 * Slow path rx queue
 * Slow path tx queue
 * Packet pool bitmap
 * Packet payload pool
 */
typedef struct _VNIC {
	// 2MBs aligned
	uint64_t	magic;

	uint32_t	id;	// VNIC unique ID (unique ID in RTOS)
	
	uint64_t	mac;
	
	uint64_t	rx_bandwidth;
	uint64_t	tx_bandwidth;
	
	uint16_t	padding_head;
	uint16_t	padding_tail;
	
	VNIC_Queue	rx;
	VNIC_Queue	tx;
	
	VNIC_Queue	srx;
	VNIC_Queue	stx;
	
	VNIC_Pool	pool;
	
	uint32_t	config;
	uint8_t		config_head[0];
	uint8_t		config_tail[0] __attribute__((__aligned__(VNIC_HEADER_SIZE)));
	
	// rx queue (8 bytes aligned)
	// tx queue (8 bytes aligned)
	// slow rx queue (8 bytes aligned)
	// slow tx queue (8 bytes aligned)
	// pool bitmap (8 bytes aligned)
	// pool (VNIC_CHUNK_SIZE(64) bytges aligned)
} VNIC;

int vnic_count();
VNIC* vnic_get(int index);
VNIC* vnic_get_by_id(uint32_t id);

Packet* vnic_alloc(VNIC* vnic, uint16_t size);
bool vnic_free(Packet* packet);

bool vnic_has_rx(VNIC* vnic);
Packet* vnic_rx(VNIC* vnic);
uint32_t vnic_rx_size(VNIC* vnic);

bool vnic_has_srx(VNIC* vnic);
Packet* vnic_srx(VNIC* vnic);
uint32_t vnic_srx_size(VNIC* vnic);

bool vnic_tx(VNIC* vnic, Packet* packet);
bool vnic_try_tx(VNIC* vnic, Packet* packet);
bool vnic_tx_dup(VNIC* vnic, Packet* packet);
bool vnic_has_tx(VNIC* vnic);
uint32_t vnic_tx_size(VNIC* vnic);

bool vnic_stx(VNIC* vnic, Packet* packet);
bool vnic_try_stx(VNIC* vnic, Packet* packet);
bool vnic_stx_dup(VNIC* vnic, Packet* packet);
bool vnic_has_stx(VNIC* vnic);
uint32_t vnic_stx_size(VNIC* vnic);

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
 * Fast path rx queue
 * Fast path tx queue
 * Slow path rx queue
 * Slow path tx queue
 * Malloc bitmap
 * Malloc pool
 *
 * @param base 2MBs aligned
 * @param size multiples of 2MBs
 * @return 0 succeed
 * @return 1 the base address is not aligned in 2MBs
 * @return 2 the size is not rounded up 2MBs
 */
int vnic_driver_init(uint32_t id, uint64_t mac, void* base, size_t size, 
		uint64_t rx_bandwidth, uint64_t tx_bandwidth, 
		uint16_t padding_head, uint16_t padding_tail, 
		uint32_t rx_queue_size, uint32_t tx_queue_size, 
		uint32_t srx_queue_size, uint32_t stx_queue_size);

bool vnic_driver_has_rx(VNIC* vnic);
bool vnic_driver_rx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2);
bool vnic_driver_rx2(VNIC* vnic, Packet* packet);

bool vnic_driver_has_srx(VNIC* vnic);
bool vnic_driver_srx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2);
bool vnic_driver_srx2(VNIC* vnic, Packet* packet);

bool vnic_driver_has_tx(VNIC* vnic);
Packet* vnic_driver_tx(VNIC* vnic);

bool vnic_driver_has_stx(VNIC* vnic);
Packet* vnic_driver_stx(VNIC* vnic);

#endif /* __VNIC_H__ */
