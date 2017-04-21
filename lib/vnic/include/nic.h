#ifndef __NIC_H__
#define __NIC_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <net/packet.h>

#define MAX_NIC_NAME_LEN	16

#define ROUNDUP(x, y)	((((x) + (y) - 1) / (y)) * (y))

#define NIC_MAX_COUNT		64
#define NIC_MAX_LINKS		8
#define NIC_CHUNK_SIZE		64
#define NIC_MAX_SIZE		(16 * 1024 * 1024)	// 16MB
#define NIC_HEADER_SIZE		(64 * 1024)		// 64KB

#define NIC_MAGIC_HEADER	0x0A38E56586468C01LL	// PacketNgin vNIC 01(version)

/**
 * @file
 * Virtual Network Interface Controller (vNIC) host API
 */

// Host API

typedef struct _NIC_Queue {
	uint32_t	base;
	uint32_t	head;
	uint32_t	tail;
	uint32_t	size;
	volatile uint8_t rlock;
	volatile uint8_t wlock;
} NIC_Queue;

typedef struct _NIC_Pool {
	uint32_t	bitmap;
	uint32_t	count;
	uint32_t	pool;
	uint32_t	index;
	uint32_t	used;
	volatile uint8_t lock;
} NIC_Pool;

/**
 * NIC Memory Map
 *
 * NIC_MAGIC_HEADER (8 bytes)
 * Metadata (bandwidth, pdding, queue, pool)
 * Config
 * Fast path rx queue
 * Fast path tx queue
 * Slow path rx queue
 * Slow path tx queue
 * Packet pool bitmap
 * Packet payload pool
 */
typedef struct _NIC {
	// 2MBs aligned
	uint64_t	magic;

	uint32_t	id;	// NIC unique ID (unique ID in RTOS)

	uint64_t	mac;

	uint64_t	rx_bandwidth;
	uint64_t	tx_bandwidth;

	uint16_t	padding_head;
	uint16_t	padding_tail;

	NIC_Queue	rx;
	NIC_Queue	tx;

	NIC_Queue	srx;
	NIC_Queue	stx;

	NIC_Pool	pool;

	uint32_t	config;
	uint8_t		config_head[0];
	uint8_t		config_tail[0] __attribute__((__aligned__(NIC_HEADER_SIZE)));

	// rx queue (8 bytes aligned)
	// tx queue (8 bytes aligned)
	// slow rx queue (8 bytes aligned)
	// slow tx queue (8 bytes aligned)
	// pool bitmap (8 bytes aligned)
	// pool (NIC_CHUNK_SIZE(64) bytges aligned)
} NIC;

int nic_count();
NIC* nic_get(int index);
NIC* nic_get_by_id(uint32_t id);

Packet* nic_alloc(NIC* nic, uint16_t size);
bool nic_free(Packet* packet);

bool queue_push(NIC* nic, NIC_Queue* queue, Packet* packet);
void* queue_pop(NIC* nic, NIC_Queue* queue);
uint32_t queue_size(NIC_Queue* queue);
bool queue_available(NIC_Queue* queue);
bool queue_empty(NIC_Queue* queue);

bool nic_has_rx(NIC* nic);
Packet* nic_rx(NIC* nic);
uint32_t nic_rx_size(NIC* nic);

bool nic_has_srx(NIC* nic);
Packet* nic_srx(NIC* nic);
uint32_t nic_srx_size(NIC* nic);

bool nic_tx(NIC* nic, Packet* packet);
bool nic_try_tx(NIC* nic, Packet* packet);
bool nic_tx_dup(NIC* nic, Packet* packet);
bool nic_has_tx(NIC* nic);
uint32_t nic_tx_size(NIC* nic);

bool nic_stx(NIC* nic, Packet* packet);
bool nic_try_stx(NIC* nic, Packet* packet);
bool nic_stx_dup(NIC* nic, Packet* packet);
bool nic_has_stx(NIC* nic);
uint32_t nic_stx_size(NIC* nic);

size_t nic_pool_used(NIC* nic);
size_t nic_pool_free(NIC* nic);
size_t nic_pool_total(NIC* nic);

uint32_t nic_config_register(NIC* nic, char* name);
uint32_t nic_config_key(NIC* nic, char* name);
bool nic_config_put(NIC* nic, uint32_t key, uint64_t value);
uint64_t nic_config_get(NIC* nic, uint32_t key);

/**
 * Initialize NIC memory map
 *
 * NIC metadata (magic, mac, ...)
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

#endif /* __NIC_H__ */
