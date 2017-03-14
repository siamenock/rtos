#include <string.h>
#include <strings.h>
#include "lock.h"
#include "vnic.h"

#define ROUNDUP(x, y)	((((x) + (y) - 1) / (y)) * (y))

VNIC* _vnics[VNIC_MAX_COUNT];

static VNIC* find_VNIC(Packet* packet) {
	VNIC* vnic = (void*)((uintptr_t)packet & ~(uintptr_t)(0x200000 - 1)); // 2MB alignment
	for(int i = 0; i < VNIC_MAX_SIZE / 0x200000  - 1 && (uintptr_t)vnic > 0; i++) {
		if(vnic->magic == VNIC_MAGIC_HEADER)
			return vnic;
		
		vnic = (void*)vnic - 0x200000;
	}
	
	return NULL;
}

int vnic_count() {
	int count = 0;
	for(int i = 0; i < VNIC_MAX_COUNT; i++) {
		if(_vnics[i] != NULL)
			count++;
	}

	return count;
}

VNIC* vnic_get(int index) {
	int count = 0;
	for(int i = 0; i < VNIC_MAX_COUNT; i++) {
		if(_vnics[i] != NULL) {
			if(index == count)
				return _vnics[i];
			else
				count++;
		}
	}

	return NULL;
}

VNIC* vnic_get_by_id(uint32_t id) {
	for(int i = 0; i < VNIC_MAX_COUNT; i++)
		if(_vnics[i] != NULL && _vnics[i]->id == id)
			return _vnics[i];
	
	return NULL;
}

Packet* vnic_alloc(VNIC* vnic, uint16_t size) {
	uint8_t* bitmap = (void*)vnic + vnic->pool.bitmap;
	uint32_t count = vnic->pool.count;
	void* pool = (void*)vnic + vnic->pool.pool;

	uint32_t size2 = sizeof(Packet) + vnic->padding_head + size + vnic->padding_tail;
	uint8_t req = (ROUNDUP(size2, VNIC_CHUNK_SIZE)) / VNIC_CHUNK_SIZE;

	lock_lock(&vnic->pool.lock);
	
	uint32_t index = vnic->pool.index;
	
	// Find tail
	uint32_t idx = 0;
	for(idx = index; idx <= count - req; idx++) {
		for(uint32_t j = 0; j < req; j++) {
			if(bitmap[idx + j] == 0) {
				continue;
			} else {
				idx += j + bitmap[idx + j];
				goto next;
			}
		}

		goto found;
next:
		;
	}
	
	// Find head
	for(idx = 0; idx < index - req; idx++) {
		for(uint32_t j = 0; j < req; j++) {
			if(bitmap[idx + j] == 0) {
				continue;
			} else {
				idx += j + bitmap[idx + j];
				goto notfound;
			}
		}

		goto found;
	}
	
notfound:
	// Not found
	lock_unlock(&vnic->pool.lock);
	return NULL;
	
found:
	vnic->pool.index = idx + req;
	for(uint32_t k = 0; k < req; k++) {
		bitmap[idx + k] = req - k;
	}
	
	vnic->pool.used += req;
	
	lock_unlock(&vnic->pool.lock);
	
	Packet* packet = pool + (idx * VNIC_CHUNK_SIZE);
	packet->time = 0;
	packet->start = 0;
	packet->end = 0;
	packet->size = (req * VNIC_CHUNK_SIZE) - sizeof(Packet);
	
	return packet;
}

bool vnic_free(Packet* packet) {
	VNIC* vnic = find_VNIC(packet);
	if(vnic == NULL)
		return false;
	
	uint8_t* bitmap = (void*)vnic + vnic->pool.bitmap;
	uint32_t count = vnic->pool.count;
	void* pool = (void*)vnic + vnic->pool.pool;
	
	uint32_t idx = ((uintptr_t)packet - (uintptr_t)pool) / VNIC_CHUNK_SIZE;
	if(idx >= count)
		return false;
	
	uint8_t req = bitmap[idx];
	if(idx + req > count)
		return false;

	for(uint32_t i = idx + req - 1; i > idx; i--) {
		bitmap[i] = 0;
}
	bitmap[idx] = 0;	// if idx is zero, it for loop will never end

	return true;
}

static bool queue_push(VNIC* vnic, VNIC_Queue* queue, Packet* packet) {
	VNIC* vnic2 = find_VNIC(packet);
	if(vnic2 == NULL)
		return false;
	
	uint64_t* array = (void*)vnic + queue->base;
	uint32_t next = (queue->tail + 1) % queue->size;
	if(queue->head != next) {
		array[queue->tail] = ((uint64_t)vnic2->id << 32) | (uint64_t)(uint32_t)((uintptr_t)packet - (uintptr_t)vnic2);
		queue->tail = next;
		
		return true;
	} else {
		return false;
	}
}

static void* queue_pop(VNIC* vnic, VNIC_Queue* queue) {
	uint64_t* array = (void*)vnic + queue->base;
	
	if(queue->head != queue->tail) {
		uint64_t tmp = array[queue->head];
		uint32_t id = (uint32_t)(tmp >> 32);
		uint32_t data = (uint32_t)tmp;
		
		array[queue->head] = 0;
		
		queue->head = (queue->head + 1) % queue->size;
		
		
		if(vnic->id == id) {
			return (void*)vnic + data;
		} else {
			vnic = vnic_get_by_id(id);
			if(vnic != NULL)
				return (void*)vnic + data;
			else
				return NULL;
		}
	} else { 
		return NULL;
	}
}

static uint32_t queue_size(VNIC_Queue* queue) {
	if(queue->tail >= queue->head)
		return queue->tail - queue->head;
	else
		return queue->size + queue->tail - queue->head;
}

static bool queue_available(VNIC_Queue* queue) {
	return queue->head != (queue->tail + 1) % queue->size;
}

static bool queue_empty(VNIC_Queue* queue) {
	return queue->head == queue->tail;
}

bool vnic_has_rx(VNIC* vnic) {
	return !queue_empty(&vnic->rx);
}

Packet* vnic_rx(VNIC* vnic) {
	lock_lock(&vnic->rx.rlock);
	Packet* packet = queue_pop(vnic, &vnic->rx);
	lock_unlock(&vnic->rx.rlock);
	
	return packet;
}

uint32_t vnic_rx_size(VNIC* vnic) {
	return queue_size(&vnic->rx);
}

bool vnic_has_srx(VNIC* vnic) {
	return !queue_empty(&vnic->srx);
}

Packet* vnic_srx(VNIC* vnic) {
	lock_lock(&vnic->srx.rlock);
	Packet* packet = queue_pop(vnic, &vnic->srx);
	lock_unlock(&vnic->srx.rlock);

	return packet;
}

uint32_t vnic_srx_size(VNIC* vnic) {
	return queue_size(&vnic->srx);
}

bool vnic_tx(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->tx.wlock);
	if(!queue_push(vnic, &vnic->tx, packet)) {
		lock_unlock(&vnic->tx.wlock);
		
		vnic_free(packet);
		return false;
	} else {
		lock_unlock(&vnic->tx.wlock);
		return true;
	}
}

bool vnic_try_tx(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->tx.wlock);
	bool result = queue_push(vnic, &vnic->tx, packet);
	lock_unlock(&vnic->tx.wlock);
	
	return result;
}

bool vnic_tx_dup(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->tx.wlock);
	if(!queue_available(&vnic->tx)) {
		lock_unlock(&vnic->tx.wlock);
		return false;
	}
	
	int len = packet->end - packet->start;
	
	Packet* packet2 = vnic_alloc(vnic, len);
	if(!packet2) {
		lock_unlock(&vnic->tx.wlock);
		return false;
	}
	
	packet2->time = packet->time;
	packet2->end = packet2->start + len;
	memcpy(packet2->buffer + packet2->start, packet->buffer + packet->start, len);
	
	if(!queue_push(vnic, &vnic->tx, packet)) {
		lock_unlock(&vnic->tx.wlock);
		
		vnic_free(packet);
		return false;
	} else {
		lock_unlock(&vnic->tx.wlock);
		return true;
	}
}

bool vnic_has_tx(VNIC* vnic) {
	return queue_available(&vnic->tx);
}

uint32_t vnic_tx_size(VNIC* vnic) {
	return queue_size(&vnic->tx);
}

bool vnic_stx(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->stx.wlock);
	if(!queue_push(vnic, &vnic->stx, packet)) {
		lock_unlock(&vnic->stx.wlock);
		vnic_free(packet);
		
		return false;
	} else {
		lock_unlock(&vnic->stx.wlock);
		return true;
	}
}

bool vnic_try_stx(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->stx.wlock);
	bool result = queue_push(vnic, &vnic->stx, packet);
	lock_unlock(&vnic->stx.wlock);

	return result;
}

bool vnic_stx_dup(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->stx.wlock);
	if(!queue_available(&vnic->stx)) {
		lock_unlock(&vnic->stx.wlock);
		return false;
	}
	
	int len = packet->end - packet->start;
	
	Packet* packet2 = vnic_alloc(vnic, len);
	if(!packet2) {
		lock_unlock(&vnic->stx.wlock);
		return false;
	}
	
	packet2->time = packet->time;
	packet2->end = packet2->start + len;
	memcpy(packet2->buffer + packet2->start, packet->buffer + packet->start, len);
	
	if(!queue_push(vnic, &vnic->stx, packet)) {
		lock_unlock(&vnic->stx.wlock);
		vnic_free(packet);
		
		return false;
	} else {
		lock_unlock(&vnic->stx.wlock);
		return true;
	}
}

bool vnic_has_stx(VNIC* vnic) {
	return queue_available(&vnic->stx);
}

uint32_t vnic_stx_size(VNIC* vnic) {
	return queue_size(&vnic->stx);
}

size_t vnic_pool_used(VNIC* vnic) {
	return vnic->pool.used * VNIC_CHUNK_SIZE;
}

size_t vnic_pool_free(VNIC* vnic) {
	return (vnic->pool.count - vnic->pool.used) * VNIC_CHUNK_SIZE;
}

size_t vnic_pool_total(VNIC* vnic) {
	return vnic->pool.count * VNIC_CHUNK_SIZE;
}

uint32_t vnic_config_register(VNIC* vnic, char* name) {
	int len = strlen(name) + 1;
	if(len > 255)
		return 0;
	
	uint8_t* n = vnic->config_head;
	int i;
	for(i = 0; i < vnic->config; i++) {
		uint8_t l = *n++;

		if(strncmp(name, (const char*)n, l) == 0)
			return 0;
		
		n += l;
	}
	
	int available = (int)(vnic->config_tail - n) - sizeof(uint64_t) * vnic->config;
	if(1 + len + sizeof(uint64_t) > available)
		return 0;
	
	*n++ = (uint8_t)len;
	memcpy(n, name, len);
	
	return ++vnic->config;
}

uint32_t vnic_config_key(VNIC* vnic, char* name) {
	int len = strlen(name) + 1;
	if(len > 255)
		return 0;
	
	uint8_t* n = vnic->config_head;
	for(int i = 0; i < vnic->config; i++) {
		uint8_t l = *n++;
		
		if(strncmp(name, (const char*)n, l) == 0)
			return i + 1;
		
		n += l;
	}
	
	return 0;
}

bool vnic_config_put(VNIC* vnic, uint32_t key, uint64_t value) {
	if(key > vnic->config)
		return false;
	
	*(uint64_t*)(vnic->config_tail - key * sizeof(uint64_t)) = value;

	return true;
}

uint64_t vnic_config_get(VNIC* vnic, uint32_t key) {
	return *(uint64_t*)(vnic->config_tail - key * sizeof(uint64_t));
}

// Driver API
int vnic_driver_init(uint32_t id, uint64_t mac, void* base, size_t size, 
		uint64_t rx_bandwidth, uint64_t tx_bandwidth, 
		uint16_t padding_head, uint16_t padding_tail, 
		uint32_t rx_queue_size, uint32_t tx_queue_size, 
		uint32_t srx_queue_size, uint32_t stx_queue_size) {
	
	if((uintptr_t)base == 0 || (uintptr_t)base % 0x200000 != 0)
		return 1;

	if(size % 0x200000 != 0)
		return 2;
	
	int index = sizeof(VNIC);
	
	VNIC* vnic = base;
	vnic->magic = VNIC_MAGIC_HEADER;
	vnic->id = id;
	vnic->mac = mac;
	vnic->rx_bandwidth = rx_bandwidth;
	vnic->tx_bandwidth = tx_bandwidth;
	vnic->padding_head = padding_head;
	vnic->padding_tail = padding_tail;
	
	vnic->rx.base = index;
	vnic->rx.head = 0;
	vnic->rx.tail = 0;
	vnic->rx.size = rx_queue_size;
	vnic->rx.rlock = 0;
	vnic->rx.wlock = 0;
	
	index += vnic->rx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	vnic->tx.base = index;
	vnic->tx.head = 0;
	vnic->tx.tail = 0;
	vnic->tx.size = tx_queue_size;
	vnic->tx.rlock = 0;
	vnic->tx.wlock = 0;
	
	index += vnic->tx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	vnic->srx.base = index;
	vnic->srx.head = 0;
	vnic->srx.tail = 0;
	vnic->srx.size = srx_queue_size;
	vnic->srx.rlock = 0;
	vnic->srx.wlock = 0;
	
	index += vnic->srx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	vnic->stx.base = index;
	vnic->stx.head = 0;
	vnic->stx.tail = 0;
	vnic->stx.size = stx_queue_size;
	vnic->stx.rlock = 0;
	vnic->stx.wlock = 0;
	
	index += vnic->stx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	vnic->pool.bitmap = index;
	index += (size - index) / VNIC_CHUNK_SIZE;
	index = ROUNDUP(index, VNIC_CHUNK_SIZE);
	vnic->pool.pool = index;
	vnic->pool.count = (size - index) / VNIC_CHUNK_SIZE;
	vnic->pool.index = 0;
	vnic->pool.used = 0;
	vnic->pool.lock = 0;
	
	vnic->config = 0;
	bzero(vnic->config_head, (size_t)((uintptr_t)vnic->config_tail - (uintptr_t)vnic->config_head));
	
	bzero(base + vnic->pool.bitmap, vnic->pool.count);

	return 0;
}

bool vnic_driver_has_rx(VNIC* vnic) {
	return queue_available(&vnic->rx);
}

bool vnic_driver_rx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&vnic->rx.wlock);
	if(queue_available(&vnic->rx)) {
		Packet* packet = vnic_alloc(vnic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&vnic->rx.wlock);
			return false;
		}
		
		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);
		
		packet->end = packet->start + size1 + size2;
		
		if(queue_push(vnic, &vnic->rx, packet)) {
			lock_unlock(&vnic->rx.wlock);
			return true;
		} else {
			lock_unlock(&vnic->rx.wlock);
			vnic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&vnic->rx.wlock);
		return false;
	}
}

bool vnic_driver_rx2(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->rx.wlock);
	if(queue_push(vnic, &vnic->rx, packet)) {
		lock_unlock(&vnic->rx.wlock);
		return true;
	} else {
		lock_unlock(&vnic->rx.wlock);
		vnic_free(packet);
		return false;
	}
}

bool vnic_driver_has_srx(VNIC* vnic) {
	return queue_available(&vnic->srx);
}

bool vnic_driver_srx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&vnic->srx.wlock);
	if(queue_available(&vnic->srx)) {
		Packet* packet = vnic_alloc(vnic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&vnic->srx.wlock);
			return false;
		}
		
		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);
		
		packet->end = packet->start + size1 + size2;
		
		if(queue_push(vnic, &vnic->srx, packet)) {
			lock_unlock(&vnic->srx.wlock);
			return true;
		} else {
			lock_unlock(&vnic->srx.wlock);
			vnic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&vnic->srx.wlock);
		return false;
	}
}

bool vnic_driver_srx2(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->srx.wlock);
	if(queue_push(vnic, &vnic->srx, packet)) {
		lock_unlock(&vnic->srx.wlock);
		return true;
	} else {
		lock_unlock(&vnic->srx.wlock);
		vnic_free(packet);
		return false;
	}
}

bool vnic_driver_has_tx(VNIC* vnic) {
	return !queue_empty(&vnic->tx);
}

Packet* vnic_driver_tx(VNIC* vnic) {
	lock_lock(&vnic->tx.rlock);
	Packet* packet = queue_pop(vnic, &vnic->tx);
	lock_unlock(&vnic->tx.rlock);

	return packet;
}

bool vnic_driver_has_stx(VNIC* vnic) {
	return !queue_empty(&vnic->stx);
}

Packet* vnic_driver_stx(VNIC* vnic) {
	lock_lock(&vnic->stx.rlock);
	Packet* packet = queue_pop(vnic, &vnic->stx);
	lock_unlock(&vnic->stx.rlock);

	return packet;
}

#if TEST

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void print_queue(VNIC_Queue* queue) {
	printf("\tbase: %d\n", queue->base);
	printf("\thead: %d\n", queue->head);
	printf("\ttail: %d\n", queue->tail);
	printf("\tsize: %d\n", queue->size);
}

static void print_config(VNIC* vnic) {	
	printf("Config count: %d\n", vnic->config);
	uint8_t* name = vnic->config_head;
	for(int i = 0; i < vnic->config; i++) {
		uint8_t len = *name++;
		printf("\t[%d] name: %s, key: %u, value: %016lx\n", i, (char*)name, i + 1, *(uint64_t*)(vnic->config_tail - (i + 1) * sizeof(uint64_t)));
		name += len;
	}
}

static void dump_config(VNIC* vnic) {
	uint8_t* p = (void*)vnic->config_head;
	int i = 0;
	while(p < vnic->config_tail) {
		printf("%02x", *p);
		if((i + 1) % 8 == 0)
			printf(" ");
		if((i + 1) % 40 == 0)
			printf("\n");
		p++;
		i++;
	}
	printf("\nconfig=%d\n", vnic->config);
}

static void dump(VNIC* vnic) {
	printf("vnic: %p\n", vnic);
	printf("magic: %lx\n", vnic->magic);
	printf("MAC: %lx\n", vnic->mac);
	printf("rx_bandwidth: %ld\n", vnic->rx_bandwidth);
	printf("tx_bandwidth: %ld\n", vnic->tx_bandwidth);
	printf("padding_head: %d\n", vnic->padding_head);
	printf("padding_tail: %d\n", vnic->padding_tail);
	printf("rx queue\n");
	print_queue(&vnic->rx);
	printf("tx queue\n");
	print_queue(&vnic->tx);
	printf("srx queue\n");
	print_queue(&vnic->srx);
	printf("tx slow_queue\n");
	print_queue(&vnic->stx);
	printf("pool\n");
	printf("\tbitmap: %d\n", vnic->pool.bitmap);
	printf("\tcount: %d\n", vnic->pool.count);
	printf("\tpool: %d\n", vnic->pool.pool);
	printf("\tindex: %d\n", vnic->pool.index);
	print_config(vnic);
}

static void dump_queue(VNIC* vnic, VNIC_Queue* queue) {
	print_queue(queue);
	uint64_t* array = (void*)vnic + queue->base;
	for(int i = 0; i < queue->size; i++) {
		printf("%016lx ", array[i]);
		if((i + 1) % 6 == 0)
			printf("\n");
	}
	printf("\n");
}

static void dump_bitmap(VNIC* vnic) {
	uint8_t* bitmap = (void*)vnic + vnic->pool.bitmap;
	uint32_t count = vnic->pool.count;
	uint32_t index = vnic->pool.index;
	for(int i = 0; i < count; i++) {
		printf("%02x", bitmap[i]);
		if((i + 1) % 8 == 0)
			printf(" ");

		if((i + 1) % 48 == 0)
			printf("\n");
	}
	printf("\n");
}

static int bitmap_used(VNIC* vnic) {
	uint8_t* bitmap = (void*)vnic + vnic->pool.bitmap;
	uint32_t count = vnic->pool.count;
	int used = 0;
	for(int i = 0; i < count; i++) {
		if(bitmap[i] != 0)
			used++;
	}

	return used;
}

uint8_t buffer[2 * 1024 * 1024] __attribute__((__aligned__(2 * 1024 * 1024)));

void pass() {
	printf("\tPASSED\n");
}

void fail(const char* format, ...) {
	fflush(stdout);
	fprintf(stderr, "\tFAILED\n");
	
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);

	printf("\n");
	
	VNIC* vnic = (VNIC*)buffer;
	
	printf("* rx\n");
	dump_queue(vnic, &vnic->rx);
	
	printf("* tx\n");
	dump_queue(vnic, &vnic->tx);
	
	printf("* srx\n");
	dump_queue(vnic, &vnic->srx);
	
	printf("* stx\n");
	dump_queue(vnic, &vnic->stx);
	
	/*
	printf("* Bitmap\n");
	dump_bitmap(vnic);
	print_config(vnic);
	*/
	
	exit(1);
}

int main(int argc, char** argv) {
	VNIC* vnic = (VNIC*)buffer;
	
	vnic_driver_init(1, 0x001122334455, vnic, 2 * 1024 * 1024,
		1000000000L, 1000000000L,
		16, 32, 128, 128, 64, 64);
	
	dump(vnic);


	int used = bitmap_used(vnic);
	printf("Pool: Check initial used: %d", used);
	if(used == 0)
		pass();
	else
		fail("initial used must be 0");
	
	
	Packet* p1 = vnic_alloc(vnic, 64);
	used = bitmap_used(vnic);
	printf("Pool: Check 64 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 2 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 2 chunks must be used");
	
	
	vnic_free(p1);
	used = bitmap_used(vnic);
	printf("Pool: Check 64 bytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");
	
	
	p1 = vnic_alloc(vnic, 512);
	used = bitmap_used(vnic);
	printf("Pool: Check 512 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 9 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 9 chunks must be used");
	
	
	vnic_free(p1);
	used = bitmap_used(vnic);
	printf("Pool: Check 512cbytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");
	
	
	p1 = vnic_alloc(vnic, 1500);
	used = bitmap_used(vnic);
	printf("Pool: Check 1500 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 25 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 25 chunks must be used");
	
	
	vnic_free(p1);
	used = bitmap_used(vnic);
	printf("Pool: Check 1500 bytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");
	
	
	p1 = vnic_alloc(vnic, 0);
	used = bitmap_used(vnic);
	printf("Pool: Check 0 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 1 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 1 chunk must be used");
	
	
	vnic_free(p1);
	used = bitmap_used(vnic);
	printf("Pool: Check 0 bytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");
	
	
	printf("Pool: Check full allocation: ");
	vnic->pool.index = 0;
	
	int max = vnic->pool.count / 25;
	Packet* ps[2048] = { NULL, };
	int i;
	for(i = 0; i < max; i++) {
		ps[i] = vnic_alloc(vnic, 1500);
		if(ps[i] == NULL)
			fail("allocation failed: index: %d", i);
	}

	used = bitmap_used(vnic);
	if(used != max * 25)
		fail("%d * 25 chunks must be used: %d", max, used);
	else
		pass();
	

	printf("Pool: Check overflow(big packet): ");
	p1 = vnic_alloc(vnic, 1500);
	if(p1 != NULL)
		fail("packet must not be allocated: %p", p1);
	else
		pass();
	
	printf("Pool: Small packets allocation from rest of small chunks: ");
	used = bitmap_used(vnic);
	while(vnic->pool.count - used > 0) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL) {
			fail("Small packet must be allocated: index: %d, packet: %p", i, ps[i]);
		}
		i++;
		used = bitmap_used(vnic);
	}
	pass();
	
	printf("Pool: Check overflow(small packet): ");
	p1 = vnic_alloc(vnic, 0);
	if(p1 != NULL)
		fail("packet must not be allocated: %p", p1);
	else
		pass();
	
	printf("Pool: Clear all: ");
	for(int j = 0; j < i; j++) {
		if(!vnic_free(ps[j]))
			fail("packet can not be freed: index: %d, packet: %p\n", j, ps[j]);
	}

	used = bitmap_used(vnic);
	if(used != 0)
		fail("0 chunk must be used: %d", used);
	else
		pass();
	

	p1 = vnic_alloc(vnic, 64);
	used = bitmap_used(vnic);
	printf("Pool: Check 64 bytes packet allocation after full allocation: packet: %p, used: %d", p1, used);
	if(used == 2 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 2 chunks must be used");
	
	
	vnic_free(p1);
	used = bitmap_used(vnic);
	printf("Pool: Check 64 bytes packet deallocation after full allocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");
	
	
	printf("rx queue: Check initial status: ");
	if(vnic_has_rx(vnic))
		fail("vnic_has_rxmust be false");
	
	p1 = vnic_rx(vnic);
	if(p1 != NULL)
		fail("vnic_rx must be NULL: %p", p1);

	uint32_t size = vnic_rx_size(vnic);
	if(size != 0)
		fail("vnic_rx_size must be 0: %d", size);
	
	if(!vnic_driver_has_rx(vnic))
		fail("vnic_driver_has_rx must be true");
	
	pass();


	printf("rx queue: push one: ");
	p1 = vnic_alloc(vnic, 64);
	if(!vnic_driver_rx2(vnic, p1))
		fail("vnic_driver_rx must be return true");
	
	if(!vnic_has_rx(vnic))
		fail("vnic_has_rx must be true");
	
	size = vnic_rx_size(vnic);
	if(size != 1)
		fail("vnic_rx_size must be 1: %d", size);
	
	pass();
	

	printf("rx queue: pop one: ");
	
	Packet* p2 = vnic_rx(vnic);
	if(p2 != p1)
		fail("vnic_rx returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_rx_size(vnic);
	if(size != 0)
		fail("vnic_rx_size must be 0: %d", size);
	
	vnic_free(p2);
	
	pass();
	

	printf("rx queue: push full: ");
	for(i = 0; i < vnic->rx.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i);
		
		if(!vnic_driver_rx2(vnic, ps[i]))
			fail("cannot push rx: count: %d, queue size: %d", i, vnic->rx.size);
	}
	
	if(vnic_driver_has_rx(vnic))
		fail("vnic_driver_has_rx must return false");
	
	size = vnic_rx_size(vnic);
	if(size != vnic->rx.size - 1)
		fail("vnic_rx_size must return %d but %d", vnic->rx.size - 1, size);
	
	if(!vnic_has_rx(vnic))
		fail("vnic_has_rx must return true");
	
	pass();
	

	printf("rx queue: overflow: ");

	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("packet allocation failed");
	
	if(vnic_driver_rx2(vnic, p1))
		fail("push overflow: count: %d, queue size: %d", i, vnic->rx.size);
	
	int used2 = bitmap_used(vnic);
	if(used != used2)
		fail("packet pool usage is changed: used: %d != %d", used, used2);
	
	if(vnic_driver_has_rx(vnic))
		fail("vnic_driver_has_rx must return false");
	
	size = vnic_rx_size(vnic);
	if(size != vnic->rx.size - 1)
		fail("vnic_rx_size must return %d but %d", vnic->rx.size - 1, size);
	
	if(!vnic_has_rx(vnic))
		fail("vnic_has_rx must return true");
	
	pass();
	

	printf("rx queue: pop: ");
	for(i = 0; i < vnic->rx.size - 1; i++) {
		Packet* p1 = vnic_rx(vnic);
		if(p1 != ps[i])
			fail("worong pointer returned: %p, expected: %p", p1, (void*)(uintptr_t)i);

		vnic_free(p1);
	}
	
	if(!vnic_driver_has_rx(vnic))
		fail("vnic_driver_has_rx must return true");
	
	size = vnic_rx_size(vnic);
	if(size != 0)
		fail("vnic_rx_size must return 0 but %d", size);
	
	if(vnic_has_rx(vnic))
		fail("vnic_has_rx must return false");
	
	pass();


	printf("rx queue: push one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("cannot alloc packet: %p\n", p1);
	
	if(!vnic_driver_rx2(vnic, p1))
		fail("vnic_driver_rx must be return true");
	
	if(!vnic_has_rx(vnic))
		fail("vnic_has_rx must be true");
	
	size = vnic_rx_size(vnic);
	if(size != 1)
		fail("vnic_rx_size must be 1: %d", size);
	
	pass();
	

	printf("rx queue: pop one after overflow: ");
	
	p2 = vnic_rx(vnic);
	if(p1 != p2)
		fail("vnic_rx returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_rx_size(vnic);
	if(size != 0)
		fail("vnic_rx_size must be 0: %d", size);
	
	vnic_free(p2);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
	
	pass();


	printf("srx queue: Check initial status: ");
	if(vnic_has_srx(vnic))
		fail("vnic_has_srx must be false");
	
	p1 = vnic_srx(vnic);
	if(p1 != NULL)
		fail("vnic_srx must be NULL: %p", p1);

	size = vnic_srx_size(vnic);
	if(size != 0)
		fail("vnic_srx_size must be 0: %d", size);
	
	if(!vnic_driver_has_srx(vnic))
		fail("vnic_driver_has_srx must be true");
	
	pass();


	printf("srx queue: push one: ");
	p1 = vnic_alloc(vnic, 64);
	if(!vnic_driver_srx2(vnic, p1))
		fail("vnic_driver_srx must be return true");
	
	if(!vnic_has_srx(vnic))
		fail("vnic_has_srx must be true");
	
	size = vnic_srx_size(vnic);
	if(size != 1)
		fail("vnic_srx_size must be 1: %d", size);
	
	pass();
	

	printf("srx queue: pop one: ");
	
	p2 = vnic_srx(vnic);
	if(p2 != p1)
		fail("vnic_srx returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_srx_size(vnic);
	if(size != 0)
		fail("vnic_srx_size must be 0: %d", size);
	
	vnic_free(p2);
	
	pass();
	

	printf("srx queue: push full: ");
	for(i = 0; i < vnic->srx.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i);
		
		if(!vnic_driver_srx2(vnic, ps[i]))
			fail("cannot push srx: count: %d, queue size: %d", i, vnic->srx.size);
	}
	
	if(vnic_driver_has_srx(vnic))
		fail("vnic_driver_has_srx must return false");
	
	size = vnic_srx_size(vnic);
	if(size != vnic->srx.size - 1)
		fail("vnic_srx_size must return %d but %d", vnic->srx.size - 1, size);
	
	if(!vnic_has_srx(vnic))
		fail("vnic_has_srx must return true");
	
	pass();
	

	printf("srx queue: overflow: ");

	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("packet allocation failed");
	
	if(vnic_driver_srx2(vnic, p1))
		fail("push overflow: count: %d, queue size: %d", i, vnic->srx.size);
	
	used2 = bitmap_used(vnic);
	if(used != used2)
		fail("packet pool usage is changed: used: %d != %d", used, used2);
	
	if(vnic_driver_has_srx(vnic))
		fail("vnic_driver_has_srx must return false");
	
	size = vnic_srx_size(vnic);
	if(size != vnic->srx.size - 1)
		fail("vnic_srx_size must return %d but %d", vnic->srx.size - 1, size);
	
	if(!vnic_has_srx(vnic))
		fail("vnic_has_srx must return true");
	
	pass();
	

	printf("srx queue: pop: ");
	for(i = 0; i < vnic->srx.size - 1; i++) {
		Packet* p1 = vnic_srx(vnic);
		if(p1 != ps[i])
			fail("worong pointer returned: %p, expected: %p", p1, (void*)(uintptr_t)i);

		vnic_free(p1);
	}
	
	if(!vnic_driver_has_srx(vnic))
		fail("vnic_driver_has_srx must return true");
	
	size = vnic_srx_size(vnic);
	if(size != 0)
		fail("vnic_srx_size must return 0 but %d", size);
	
	if(vnic_has_srx(vnic))
		fail("vnic_has_srx must return false");
	
	pass();


	printf("srx queue: push one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("cannot alloc packet: %p\n", p1);
	
	if(!vnic_driver_srx2(vnic, p1))
		fail("vnic_driver_srx must be return true");
	
	if(!vnic_has_srx(vnic))
		fail("vnic_has_srx must be true");
	
	size = vnic_srx_size(vnic);
	if(size != 1)
		fail("vnic_srx_size must be 1: %d", size);
	
	pass();
	

	printf("srx queue: pop one after overflow: ");
	
	p2 = vnic_srx(vnic);
	if(p1 != p2)
		fail("vnic_srx returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_srx_size(vnic);
	if(size != 0)
		fail("vnic_srx_size must be 0: %d", size);
	
	vnic_free(p2);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
	
	pass();



	printf("tx queue: Check initial state: ");

	size = vnic_tx_size(vnic);
	if(size != 0)
		fail("vnic_tx_size must be 0: %d", size);
	
	if(!vnic_has_tx(vnic))
		fail("vnic_has_tx must be true");
	
	if(vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must false");

	pass();
	

	printf("tx queue: tx one: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_tx(vnic, p1))
		fail("vnic_tx must be true");
	
	size = vnic_tx_size(vnic);
	if(size != 1)
		fail("vnic_tx_size must be 1: %d", size);
	
	if(!vnic_has_tx(vnic))
		fail("vnic_has_tx must be true");
	
	if(!vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must true");

	pass();
	

	printf("tx queue: send one: ");
	p1 = vnic_driver_tx(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_tx_size(vnic);
	if(size != 0)
		fail("vnic_tx_size must be 0: %d", size);
	
	if(!vnic_has_tx(vnic))
		fail("vnic_has_tx must be true");
	
	if(vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must false");
	
	pass();


	printf("tx queue: tx full: ");
	for(i = 0; i < vnic->tx.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i + 1);
		
		if(!vnic_tx(vnic, ps[i]))
			fail("cannot tx packet: index: %d", i);
	}

	size = vnic_tx_size(vnic);
	if(size != vnic->tx.size - 1)
		fail("vnic_tx_size must be %d: %d", vnic->tx.size - 1, size);
	
	if(vnic_has_tx(vnic))
		fail("vnic_has_tx must be false");
	
	if(!vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must true");
	
	pass();


	printf("tx queue: overflow: ");
	
	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	
	if(vnic_tx(vnic, p1))
		fail("vnic_tx overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet not freed on overflow %d != %d", used, used2);
		
	if(vnic_try_tx(vnic, p1))
		fail("vnic_try_tx overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet freed on try_tx %d != %d", used, used2);
	
	if(vnic_tx_dup(vnic, p1))
		fail("vnic_tx_dup overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet allocated on overflow %d != %d", used, used2);

	size = vnic_tx_size(vnic);
	if(size != vnic->tx.size - 1)
		fail("vnic_tx_size must be %d: %d", vnic->tx.size - 1, size);
	
	if(vnic_has_tx(vnic))
		fail("vnic_has_tx must be false");
	
	if(!vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must true");
	
	pass();
	
	
	printf("tx queue: send all: ");
	for(i = 0; i < vnic->tx.size - 1; i++) {
		p1 = vnic_driver_tx(vnic);
		if(p1 != ps[i])
			fail("wrong pointer returned: %p != %p", ps[i], p1);
		
		if(!vnic_free(p1))
			fail("cannot free packet: %p", p1);
	}

	size = vnic_tx_size(vnic);
	if(size != 0)
		fail("vnic_tx_size must be 0: %d", 0, size);
	
	if(!vnic_has_tx(vnic))
		fail("vnic_has_tx must be true");
	
	if(vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must false");
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
		
	pass();


	printf("tx queue: tx one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_tx(vnic, p1))
		fail("vnic_tx must be true");
	
	size = vnic_tx_size(vnic);
	if(size != 1)
		fail("vnic_tx_size must be 1: %d", size);
	
	if(!vnic_has_tx(vnic))
		fail("vnic_has_tx must be true");
	
	if(!vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must true");

	pass();
	

	printf("tx queue: send one after overflow: ");
	p1 = vnic_driver_tx(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_tx_size(vnic);
	if(size != 0)
		fail("vnic_tx_size must be 0: %d", size);
	
	if(!vnic_has_tx(vnic))
		fail("vnic_has_tx must be true");
	
	if(vnic_driver_has_tx(vnic))
		fail("vnic_driver_has_tx must false");
	
	pass();


	printf("Stx queue: Check initial state: ");

	size = vnic_stx_size(vnic);
	if(size != 0)
		fail("vnic_stx_size must be 0: %d", size);
	
	if(!vnic_has_stx(vnic))
		fail("vnic_has_stx must be true");
	
	if(vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must false");

	pass();
	

	printf("Stx queue: stx one: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_stx(vnic, p1))
		fail("vnic_stx must be true");
	
	size = vnic_stx_size(vnic);
	if(size != 1)
		fail("vnic_stx_size must be 1: %d", size);
	
	if(!vnic_has_stx(vnic))
		fail("vnic_has_stx must be true");
	
	if(!vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must true");

	pass();
	

	printf("Stx queue: send one: ");
	p1 = vnic_driver_stx(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_stx_size(vnic);
	if(size != 0)
		fail("vnic_stx_size must be 0: %d", size);
	
	if(!vnic_has_stx(vnic))
		fail("vnic_has_stx must be true");
	
	if(vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must false");
	
	pass();


	printf("Stx queue: stx full: ");
	for(i = 0; i < vnic->stx.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i + 1);
		
		if(!vnic_stx(vnic, ps[i]))
			fail("cannot stx packet: index: %d", i);
	}

	size = vnic_stx_size(vnic);
	if(size != vnic->stx.size - 1)
		fail("vnic_stx_size must be %d: %d", vnic->stx.size - 1, size);
	
	if(vnic_has_stx(vnic))
		fail("vnic_has_stx must be false");
	
	if(!vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must true");
	
	pass();


	printf("Stx queue: overflow: ");
	
	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	
	if(vnic_stx(vnic, p1))
		fail("vnic_stx overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet not freed on overflow %d != %d", used, used2);
		
	if(vnic_try_stx(vnic, p1))
		fail("vnic_try_stx overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet freed on try_stx %d != %d", used, used2);
	
	if(vnic_stx_dup(vnic, p1))
		fail("vnic_stx_dup overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet allocated on overflow %d != %d", used, used2);

	size = vnic_stx_size(vnic);
	if(size != vnic->stx.size - 1)
		fail("vnic_stx_size must be %d: %d", vnic->stx.size - 1, size);
	
	if(vnic_has_stx(vnic))
		fail("vnic_has_stx must be false");
	
	if(!vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must true");
	
	pass();
	
	
	printf("Stx queue: send all: ");
	for(i = 0; i < vnic->stx.size - 1; i++) {
		p1 = vnic_driver_stx(vnic);
		if(p1 != ps[i])
			fail("wrong pointer returned: %p != %p", ps[i], p1);
		
		if(!vnic_free(p1))
			fail("cannot free packet: %p", p1);
	}

	size = vnic_stx_size(vnic);
	if(size != 0)
		fail("vnic_stx_size must be 0: %d", 0, size);
	
	if(!vnic_has_stx(vnic))
		fail("vnic_has_stx must be true");
	
	if(vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must false");
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
		
	pass();


	printf("Stx queue: stx one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_stx(vnic, p1))
		fail("vnic_stx must be true");
	
	size = vnic_stx_size(vnic);
	if(size != 1)
		fail("vnic_stx_size must be 1: %d", size);
	
	if(!vnic_has_stx(vnic))
		fail("vnic_has_stx must be true");
	
	if(!vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must true");

	pass();
	

	printf("Stx queue: send one after overflow: ");
	p1 = vnic_driver_stx(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_stx_size(vnic);
	if(size != 0)
		fail("vnic_stx_size must be 0: %d", size);
	
	if(!vnic_has_stx(vnic))
		fail("vnic_has_stx must be true");
	
	if(vnic_driver_has_stx(vnic))
		fail("vnic_driver_has_stx must false");
	
	pass();
	

	printf("Config: register one: ");
	uint32_t key = vnic_config_register(vnic, "config");
	if(key == 0)
		fail("cannot register config: %u", key);
	
	pass();
	
	
	printf("Config: key: ");
	uint32_t key2 = vnic_config_key(vnic, "config");
	if(key != key2)
		fail("cannot get key: %u != %u", key, key2);
	
	pass();
	
	
	printf("Config: put: ");
	if(!vnic_config_put(vnic, key, 0x1234567890abcdef))
		fail("cannot put config");
	
	pass();
	

	printf("Config: put with illegal key: ");
	if(vnic_config_put(vnic, key + 1, 0x1122334455667788))
		fail("illegal key put: %d", key + 1);
	
	pass();
	
	printf("Config: get: ");
	uint64_t config = vnic_config_get(vnic, key);
	if(config != 0x1234567890abcdef)
		fail("config returns wrong value: %lx != %lx", config, 0x1234567890abcdef);
	
	pass();
	

	printf("Config: put full: ");
	uint32_t available = (uint32_t)((uintptr_t)vnic->config_tail - (uintptr_t)vnic->config_head);
	available -= 1 + 7 + 8;
	
	i = 1;
	char name[9] = { 0, };
	while(available > 1 + 9 + 8) {
		sprintf(name, "%08x", i);
		key = vnic_config_register(vnic, name);
		if(key == 0)
			fail("cannot register config: '%s'", name);

		if(!vnic_config_put(vnic, key, i))
			fail("cannot put config: %s %d", name, i);
		
		available -= 1 + 9 + 8;
		i++;
	}
	printf("available: %d", available);
	
	pass();
	

	printf("Config: overflow: ");
	key = vnic_config_register(vnic, "hello");
	if(key != 0)
		fail("overflow: %u", key);
	
	pass();
	

	printf("Config: get full: ");
	for(int j = 1; j < i; j++) {
		sprintf(name, "%08x", j);
		key = vnic_config_key(vnic, name);
		if(key == 0)
			fail("cannot get key: '%s'", name);

		uint64_t value = vnic_config_get(vnic, key);
		
		if(value != j)
			fail("wrong config value returned: %ld !=  %ld", i, j);
	}

	pass();
	
	return 0;
}

#endif /* TEST */
