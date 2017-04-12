#include <string.h>
#include <strings.h>
#include "lock.h"
#include "nic.h"

#define ROUNDUP(x, y)	((((x) + (y) - 1) / (y)) * (y))

NIC* _nics[NIC_MAX_COUNT];

static NIC* find_NIC(Packet* packet) {
	NIC* nic = (void*)((uintptr_t)packet & ~(uintptr_t)(0x200000 - 1)); // 2MB alignment
	for(int i = 0; i < NIC_MAX_SIZE / 0x200000  - 1 && (uintptr_t)nic > 0; i++) {
		if(nic->magic == NIC_MAGIC_HEADER)
			return nic;

		nic = (void*)nic - 0x200000;
	}

	return NULL;
}

int nic_count() {
	int count = 0;
	for(int i = 0; i < NIC_MAX_COUNT; i++) {
		if(_nics[i] != NULL)
			count++;
	}

	return count;
}

NIC* nic_get(int index) {
	int count = 0;
	for(int i = 0; i < NIC_MAX_COUNT; i++) {
		if(_nics[i] != NULL) {
			if(index == count)
				return _nics[i];
			else
				count++;
		}
	}

	return NULL;
}

NIC* nic_get_by_id(uint32_t id) {
	for(int i = 0; i < NIC_MAX_COUNT; i++)
		if(_nics[i] != NULL && _nics[i]->id == id)
			return _nics[i];

	return NULL;
}

Packet* nic_alloc(NIC* nic, uint16_t size) {
	uint8_t* bitmap = (void*)nic + nic->pool.bitmap;
	uint32_t count = nic->pool.count;
	void* pool = (void*)nic + nic->pool.pool;

	uint32_t size2 = sizeof(Packet) + nic->padding_head + size + nic->padding_tail;
	uint8_t req = (ROUNDUP(size2, NIC_CHUNK_SIZE)) / NIC_CHUNK_SIZE;

	lock_lock(&nic->pool.lock);

	uint32_t index = nic->pool.index;

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
	lock_unlock(&nic->pool.lock);
	return NULL;

found:
	nic->pool.index = idx + req;
	for(uint32_t k = 0; k < req; k++) {
		bitmap[idx + k] = req - k;
	}

	nic->pool.used += req;

	lock_unlock(&nic->pool.lock);

	Packet* packet = pool + (idx * NIC_CHUNK_SIZE);
	packet->time = 0;
	packet->start = 0;
	packet->end = 0;
	packet->size = (req * NIC_CHUNK_SIZE) - sizeof(Packet);

	return packet;
}

bool nic_free(Packet* packet) {
	NIC* nic = find_NIC(packet);
	if(nic == NULL)
		return false;

	uint8_t* bitmap = (void*)nic + nic->pool.bitmap;
	uint32_t count = nic->pool.count;
	void* pool = (void*)nic + nic->pool.pool;

	uint32_t idx = ((uintptr_t)packet - (uintptr_t)pool) / NIC_CHUNK_SIZE;
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

static bool queue_push(NIC* nic, NIC_Queue* queue, Packet* packet) {
	NIC* nic2 = find_NIC(packet);
	if(nic2 == NULL)
		return false;

	uint64_t* array = (void*)nic + queue->base;
	uint32_t next = (queue->tail + 1) % queue->size;
	if(queue->head != next) {
		array[queue->tail] = ((uint64_t)nic2->id << 32) | (uint64_t)(uint32_t)((uintptr_t)packet - (uintptr_t)nic2);
		queue->tail = next;

		return true;
	} else {
		return false;
	}
}

static void* queue_pop(NIC* nic, NIC_Queue* queue) {
	uint64_t* array = (void*)nic + queue->base;

	if(queue->head != queue->tail) {
		uint64_t tmp = array[queue->head];
		uint32_t id = (uint32_t)(tmp >> 32);
		uint32_t data = (uint32_t)tmp;

		array[queue->head] = 0;

		queue->head = (queue->head + 1) % queue->size;


		if(nic->id == id) {
			return (void*)nic + data;
		} else {
			nic = nic_get_by_id(id);
			if(nic != NULL)
				return (void*)nic + data;
			else
				return NULL;
		}
	} else {
		return NULL;
	}
}

static uint32_t queue_size(NIC_Queue* queue) {
	if(queue->tail >= queue->head)
		return queue->tail - queue->head;
	else
		return queue->size + queue->tail - queue->head;
}

static bool queue_available(NIC_Queue* queue) {
	return queue->head != (queue->tail + 1) % queue->size;
}

static bool queue_empty(NIC_Queue* queue) {
	return queue->head == queue->tail;
}

bool nic_has_rx(NIC* nic) {
	return !queue_empty(&nic->rx);
}

Packet* nic_rx(NIC* nic) {
	lock_lock(&nic->rx.rlock);
	Packet* packet = queue_pop(nic, &nic->rx);
	lock_unlock(&nic->rx.rlock);

	return packet;
}

uint32_t nic_rx_size(NIC* nic) {
	return queue_size(&nic->rx);
}

bool nic_has_srx(NIC* nic) {
	return !queue_empty(&nic->srx);
}

Packet* nic_srx(NIC* nic) {
	lock_lock(&nic->srx.rlock);
	Packet* packet = queue_pop(nic, &nic->srx);
	lock_unlock(&nic->srx.rlock);

	return packet;
}

uint32_t nic_srx_size(NIC* nic) {
	return queue_size(&nic->srx);
}

bool nic_tx(NIC* nic, Packet* packet) {
	lock_lock(&nic->tx.wlock);
	if(!queue_push(nic, &nic->tx, packet)) {
		lock_unlock(&nic->tx.wlock);

		nic_free(packet);
		return false;
	} else {
		lock_unlock(&nic->tx.wlock);
		return true;
	}
}

bool nic_try_tx(NIC* nic, Packet* packet) {
	lock_lock(&nic->tx.wlock);
	bool result = queue_push(nic, &nic->tx, packet);
	lock_unlock(&nic->tx.wlock);

	return result;
}

bool nic_tx_dup(NIC* nic, Packet* packet) {
	lock_lock(&nic->tx.wlock);
	if(!queue_available(&nic->tx)) {
		lock_unlock(&nic->tx.wlock);
		return false;
	}

	int len = packet->end - packet->start;

	Packet* packet2 = nic_alloc(nic, len);
	if(!packet2) {
		lock_unlock(&nic->tx.wlock);
		return false;
	}

	packet2->time = packet->time;
	packet2->end = packet2->start + len;
	memcpy(packet2->buffer + packet2->start, packet->buffer + packet->start, len);

	if(!queue_push(nic, &nic->tx, packet)) {
		lock_unlock(&nic->tx.wlock);

		nic_free(packet);
		return false;
	} else {
		lock_unlock(&nic->tx.wlock);
		return true;
	}
}

bool nic_has_tx(NIC* nic) {
	return queue_available(&nic->tx);
}

uint32_t nic_tx_size(NIC* nic) {
	return queue_size(&nic->tx);
}

bool nic_stx(NIC* nic, Packet* packet) {
	lock_lock(&nic->stx.wlock);
	if(!queue_push(nic, &nic->stx, packet)) {
		lock_unlock(&nic->stx.wlock);
		nic_free(packet);

		return false;
	} else {
		lock_unlock(&nic->stx.wlock);
		return true;
	}
}

bool nic_try_stx(NIC* nic, Packet* packet) {
	lock_lock(&nic->stx.wlock);
	bool result = queue_push(nic, &nic->stx, packet);
	lock_unlock(&nic->stx.wlock);

	return result;
}

bool nic_stx_dup(NIC* nic, Packet* packet) {
	lock_lock(&nic->stx.wlock);
	if(!queue_available(&nic->stx)) {
		lock_unlock(&nic->stx.wlock);
		return false;
	}

	int len = packet->end - packet->start;

	Packet* packet2 = nic_alloc(nic, len);
	if(!packet2) {
		lock_unlock(&nic->stx.wlock);
		return false;
	}

	packet2->time = packet->time;
	packet2->end = packet2->start + len;
	memcpy(packet2->buffer + packet2->start, packet->buffer + packet->start, len);

	if(!queue_push(nic, &nic->stx, packet)) {
		lock_unlock(&nic->stx.wlock);
		nic_free(packet);

		return false;
	} else {
		lock_unlock(&nic->stx.wlock);
		return true;
	}
}

bool nic_has_stx(NIC* nic) {
	return queue_available(&nic->stx);
}

uint32_t nic_stx_size(NIC* nic) {
	return queue_size(&nic->stx);
}

size_t nic_pool_used(NIC* nic) {
	return nic->pool.used * NIC_CHUNK_SIZE;
}

size_t nic_pool_free(NIC* nic) {
	return (nic->pool.count - nic->pool.used) * NIC_CHUNK_SIZE;
}

size_t nic_pool_total(NIC* nic) {
	return nic->pool.count * NIC_CHUNK_SIZE;
}

uint32_t nic_config_register(NIC* nic, char* name) {
	int len = strlen(name) + 1;
	if(len > 255)
		return 0;

	uint8_t* n = nic->config_head;
	int i;
	for(i = 0; i < nic->config; i++) {
		uint8_t l = *n++;

		if(strncmp(name, (const char*)n, l) == 0)
			return 0;

		n += l;
	}

	int available = (int)(nic->config_tail - n) - sizeof(uint64_t) * nic->config;
	if(1 + len + sizeof(uint64_t) > available)
		return 0;

	*n++ = (uint8_t)len;
	memcpy(n, name, len);

	return ++nic->config;
}

uint32_t nic_config_key(NIC* nic, char* name) {
	int len = strlen(name) + 1;
	if(len > 255)
		return 0;

	uint8_t* n = nic->config_head;
	for(int i = 0; i < nic->config; i++) {
		uint8_t l = *n++;

		if(strncmp(name, (const char*)n, l) == 0)
			return i + 1;

		n += l;
	}

	return 0;
}

bool nic_config_put(NIC* nic, uint32_t key, uint64_t value) {
	if(key > nic->config)
		return false;

	*(uint64_t*)(nic->config_tail - key * sizeof(uint64_t)) = value;

	return true;
}

uint64_t nic_config_get(NIC* nic, uint32_t key) {
	return *(uint64_t*)(nic->config_tail - key * sizeof(uint64_t));
}

// Driver API
int nic_driver_init(uint32_t id, uint64_t mac, void* base, size_t size,
		uint64_t rx_bandwidth, uint64_t tx_bandwidth,
		uint16_t padding_head, uint16_t padding_tail,
		uint32_t rx_queue_size, uint32_t tx_queue_size,
		uint32_t srx_queue_size, uint32_t stx_queue_size) {

	if((uintptr_t)base == 0 || (uintptr_t)base % 0x200000 != 0)
		return 1;

	if(size % 0x200000 != 0)
		return 2;

	int index = sizeof(NIC);

	NIC* nic = base;
	nic->magic = NIC_MAGIC_HEADER;
	nic->id = id;
	nic->mac = mac;
	nic->rx_bandwidth = rx_bandwidth;
	nic->tx_bandwidth = tx_bandwidth;
	nic->padding_head = padding_head;
	nic->padding_tail = padding_tail;

	nic->rx.base = index;
	nic->rx.head = 0;
	nic->rx.tail = 0;
	nic->rx.size = rx_queue_size;
	nic->rx.rlock = 0;
	nic->rx.wlock = 0;

	index += nic->rx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->tx.base = index;
	nic->tx.head = 0;
	nic->tx.tail = 0;
	nic->tx.size = tx_queue_size;
	nic->tx.rlock = 0;
	nic->tx.wlock = 0;

	index += nic->tx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->srx.base = index;
	nic->srx.head = 0;
	nic->srx.tail = 0;
	nic->srx.size = srx_queue_size;
	nic->srx.rlock = 0;
	nic->srx.wlock = 0;

	index += nic->srx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->stx.base = index;
	nic->stx.head = 0;
	nic->stx.tail = 0;
	nic->stx.size = stx_queue_size;
	nic->stx.rlock = 0;
	nic->stx.wlock = 0;

	index += nic->stx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->pool.bitmap = index;
	index += (size - index) / NIC_CHUNK_SIZE;
	index = ROUNDUP(index, NIC_CHUNK_SIZE);
	nic->pool.pool = index;
	nic->pool.count = (size - index) / NIC_CHUNK_SIZE;
	nic->pool.index = 0;
	nic->pool.used = 0;
	nic->pool.lock = 0;

	nic->config = 0;
	bzero(nic->config_head, (size_t)((uintptr_t)nic->config_tail - (uintptr_t)nic->config_head));

	bzero(base + nic->pool.bitmap, nic->pool.count);

	return 0;
}

bool nic_driver_has_rx(NIC* nic) {
	return queue_available(&nic->rx);
}

bool nic_driver_rx(NIC* nic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&nic->rx.wlock);
	if(queue_available(&nic->rx)) {
		Packet* packet = nic_alloc(nic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&nic->rx.wlock);
			return false;
		}

		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);

		packet->end = packet->start + size1 + size2;

		if(queue_push(nic, &nic->rx, packet)) {
			lock_unlock(&nic->rx.wlock);
			return true;
		} else {
			lock_unlock(&nic->rx.wlock);
			nic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&nic->rx.wlock);
		return false;
	}
}

bool nic_driver_rx2(NIC* nic, Packet* packet) {
	lock_lock(&nic->rx.wlock);
	if(queue_push(nic, &nic->rx, packet)) {
		lock_unlock(&nic->rx.wlock);
		return true;
	} else {
		lock_unlock(&nic->rx.wlock);
		nic_free(packet);
		return false;
	}
}

bool nic_driver_has_srx(NIC* nic) {
	return queue_available(&nic->srx);
}

bool nic_driver_srx(NIC* nic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&nic->srx.wlock);
	if(queue_available(&nic->srx)) {
		Packet* packet = nic_alloc(nic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&nic->srx.wlock);
			return false;
		}

		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);

		packet->end = packet->start + size1 + size2;

		if(queue_push(nic, &nic->srx, packet)) {
			lock_unlock(&nic->srx.wlock);
			return true;
		} else {
			lock_unlock(&nic->srx.wlock);
			nic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&nic->srx.wlock);
		return false;
	}
}

bool nic_driver_srx2(NIC* nic, Packet* packet) {
	lock_lock(&nic->srx.wlock);
	if(queue_push(nic, &nic->srx, packet)) {
		lock_unlock(&nic->srx.wlock);
		return true;
	} else {
		lock_unlock(&nic->srx.wlock);
		nic_free(packet);
		return false;
	}
}

bool nic_driver_has_tx(NIC* nic) {
	return !queue_empty(&nic->tx);
}

Packet* nic_driver_tx(NIC* nic) {
	lock_lock(&nic->tx.rlock);
	Packet* packet = queue_pop(nic, &nic->tx);
	lock_unlock(&nic->tx.rlock);

	return packet;
}

bool nic_driver_has_stx(NIC* nic) {
	return !queue_empty(&nic->stx);
}

Packet* nic_driver_stx(NIC* nic) {
	lock_lock(&nic->stx.rlock);
	Packet* packet = queue_pop(nic, &nic->stx);
	lock_unlock(&nic->stx.rlock);

	return packet;
}

#if TEST

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void print_queue(NIC_Queue* queue) {
	printf("\tbase: %d\n", queue->base);
	printf("\thead: %d\n", queue->head);
	printf("\ttail: %d\n", queue->tail);
	printf("\tsize: %d\n", queue->size);
}

static void print_config(NIC* nic) {
	printf("Config count: %d\n", nic->config);
	uint8_t* name = nic->config_head;
	for(int i = 0; i < nic->config; i++) {
		uint8_t len = *name++;
		printf("\t[%d] name: %s, key: %u, value: %016lx\n", i, (char*)name, i + 1, *(uint64_t*)(nic->config_tail - (i + 1) * sizeof(uint64_t)));
		name += len;
	}
}

static void dump_config(NIC* nic) {
	uint8_t* p = (void*)nic->config_head;
	int i = 0;
	while(p < nic->config_tail) {
		printf("%02x", *p);
		if((i + 1) % 8 == 0)
			printf(" ");
		if((i + 1) % 40 == 0)
			printf("\n");
		p++;
		i++;
	}
	printf("\nconfig=%d\n", nic->config);
}

static void dump(NIC* nic) {
	printf("nic: %p\n", nic);
	printf("magic: %lx\n", nic->magic);
	printf("MAC: %lx\n", nic->mac);
	printf("rx_bandwidth: %ld\n", nic->rx_bandwidth);
	printf("tx_bandwidth: %ld\n", nic->tx_bandwidth);
	printf("padding_head: %d\n", nic->padding_head);
	printf("padding_tail: %d\n", nic->padding_tail);
	printf("rx queue\n");
	print_queue(&nic->rx);
	printf("tx queue\n");
	print_queue(&nic->tx);
	printf("srx queue\n");
	print_queue(&nic->srx);
	printf("tx slow_queue\n");
	print_queue(&nic->stx);
	printf("pool\n");
	printf("\tbitmap: %d\n", nic->pool.bitmap);
	printf("\tcount: %d\n", nic->pool.count);
	printf("\tpool: %d\n", nic->pool.pool);
	printf("\tindex: %d\n", nic->pool.index);
	print_config(nic);
}

static void dump_queue(NIC* nic, NIC_Queue* queue) {
	print_queue(queue);
	uint64_t* array = (void*)nic + queue->base;
	for(int i = 0; i < queue->size; i++) {
		printf("%016lx ", array[i]);
		if((i + 1) % 6 == 0)
			printf("\n");
	}
	printf("\n");
}

static void dump_bitmap(NIC* nic) {
	uint8_t* bitmap = (void*)nic + nic->pool.bitmap;
	uint32_t count = nic->pool.count;
	uint32_t index = nic->pool.index;
	for(int i = 0; i < count; i++) {
		printf("%02x", bitmap[i]);
		if((i + 1) % 8 == 0)
			printf(" ");

		if((i + 1) % 48 == 0)
			printf("\n");
	}
	printf("\n");
}

static int bitmap_used(NIC* nic) {
	uint8_t* bitmap = (void*)nic + nic->pool.bitmap;
	uint32_t count = nic->pool.count;
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

	NIC* nic = (NIC*)buffer;

	printf("* rx\n");
	dump_queue(nic, &nic->rx);

	printf("* tx\n");
	dump_queue(nic, &nic->tx);

	printf("* srx\n");
	dump_queue(nic, &nic->srx);

	printf("* stx\n");
	dump_queue(nic, &nic->stx);

	/*
	printf("* Bitmap\n");
	dump_bitmap(nic);
	print_config(nic);
	*/

	exit(1);
}

int main(int argc, char** argv) {
	NIC* nic = (NIC*)buffer;

	nic_driver_init(1, 0x001122334455, nic, 2 * 1024 * 1024,
		1000000000L, 1000000000L,
		16, 32, 128, 128, 64, 64);

	dump(nic);


	int used = bitmap_used(nic);
	printf("Pool: Check initial used: %d", used);
	if(used == 0)
		pass();
	else
		fail("initial used must be 0");


	Packet* p1 = nic_alloc(nic, 64);
	used = bitmap_used(nic);
	printf("Pool: Check 64 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 2 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 2 chunks must be used");


	nic_free(p1);
	used = bitmap_used(nic);
	printf("Pool: Check 64 bytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");


	p1 = nic_alloc(nic, 512);
	used = bitmap_used(nic);
	printf("Pool: Check 512 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 9 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 9 chunks must be used");


	nic_free(p1);
	used = bitmap_used(nic);
	printf("Pool: Check 512cbytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");


	p1 = nic_alloc(nic, 1500);
	used = bitmap_used(nic);
	printf("Pool: Check 1500 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 25 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 25 chunks must be used");


	nic_free(p1);
	used = bitmap_used(nic);
	printf("Pool: Check 1500 bytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");


	p1 = nic_alloc(nic, 0);
	used = bitmap_used(nic);
	printf("Pool: Check 0 bytes packet allocation: packet: %p, used: %d", p1, used);
	if(used == 1 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 1 chunk must be used");


	nic_free(p1);
	used = bitmap_used(nic);
	printf("Pool: Check 0 bytes packet deallocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");


	printf("Pool: Check full allocation: ");
	nic->pool.index = 0;

	int max = nic->pool.count / 25;
	Packet* ps[2048] = { NULL, };
	int i;
	for(i = 0; i < max; i++) {
		ps[i] = nic_alloc(nic, 1500);
		if(ps[i] == NULL)
			fail("allocation failed: index: %d", i);
	}

	used = bitmap_used(nic);
	if(used != max * 25)
		fail("%d * 25 chunks must be used: %d", max, used);
	else
		pass();


	printf("Pool: Check overflow(big packet): ");
	p1 = nic_alloc(nic, 1500);
	if(p1 != NULL)
		fail("packet must not be allocated: %p", p1);
	else
		pass();

	printf("Pool: Small packets allocation from rest of small chunks: ");
	used = bitmap_used(nic);
	while(nic->pool.count - used > 0) {
		ps[i] = nic_alloc(nic, 0);
		if(ps[i] == NULL) {
			fail("Small packet must be allocated: index: %d, packet: %p", i, ps[i]);
		}
		i++;
		used = bitmap_used(nic);
	}
	pass();

	printf("Pool: Check overflow(small packet): ");
	p1 = nic_alloc(nic, 0);
	if(p1 != NULL)
		fail("packet must not be allocated: %p", p1);
	else
		pass();

	printf("Pool: Clear all: ");
	for(int j = 0; j < i; j++) {
		if(!nic_free(ps[j]))
			fail("packet can not be freed: index: %d, packet: %p\n", j, ps[j]);
	}

	used = bitmap_used(nic);
	if(used != 0)
		fail("0 chunk must be used: %d", used);
	else
		pass();


	p1 = nic_alloc(nic, 64);
	used = bitmap_used(nic);
	printf("Pool: Check 64 bytes packet allocation after full allocation: packet: %p, used: %d", p1, used);
	if(used == 2 && p1 != NULL)
		pass();
	else
		fail("packet must not be NULL and 2 chunks must be used");


	nic_free(p1);
	used = bitmap_used(nic);
	printf("Pool: Check 64 bytes packet deallocation after full allocation: used: %d", used);
	if(used == 0)
		pass();
	else
		fail("0 chunks must be used");


	printf("rx queue: Check initial status: ");
	if(nic_has_rx(nic))
		fail("nic_has_rxmust be false");

	p1 = nic_rx(nic);
	if(p1 != NULL)
		fail("nic_rx must be NULL: %p", p1);

	uint32_t size = nic_rx_size(nic);
	if(size != 0)
		fail("nic_rx_size must be 0: %d", size);

	if(!nic_driver_has_rx(nic))
		fail("nic_driver_has_rx must be true");

	pass();


	printf("rx queue: push one: ");
	p1 = nic_alloc(nic, 64);
	if(!nic_driver_rx2(nic, p1))
		fail("nic_driver_rx must be return true");

	if(!nic_has_rx(nic))
		fail("nic_has_rx must be true");

	size = nic_rx_size(nic);
	if(size != 1)
		fail("nic_rx_size must be 1: %d", size);

	pass();


	printf("rx queue: pop one: ");

	Packet* p2 = nic_rx(nic);
	if(p2 != p1)
		fail("nic_rx returned wrong pointer: %p != %p", p1, p2);

	size = nic_rx_size(nic);
	if(size != 0)
		fail("nic_rx_size must be 0: %d", size);

	nic_free(p2);

	pass();


	printf("rx queue: push full: ");
	for(i = 0; i < nic->rx.size - 1; i++) {
		ps[i] = nic_alloc(nic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i);

		if(!nic_driver_rx2(nic, ps[i]))
			fail("cannot push rx: count: %d, queue size: %d", i, nic->rx.size);
	}

	if(nic_driver_has_rx(nic))
		fail("nic_driver_has_rx must return false");

	size = nic_rx_size(nic);
	if(size != nic->rx.size - 1)
		fail("nic_rx_size must return %d but %d", nic->rx.size - 1, size);

	if(!nic_has_rx(nic))
		fail("nic_has_rx must return true");

	pass();


	printf("rx queue: overflow: ");

	used = bitmap_used(nic);

	p1 = nic_alloc(nic, 0);
	if(p1 == NULL)
		fail("packet allocation failed");

	if(nic_driver_rx2(nic, p1))
		fail("push overflow: count: %d, queue size: %d", i, nic->rx.size);

	int used2 = bitmap_used(nic);
	if(used != used2)
		fail("packet pool usage is changed: used: %d != %d", used, used2);

	if(nic_driver_has_rx(nic))
		fail("nic_driver_has_rx must return false");

	size = nic_rx_size(nic);
	if(size != nic->rx.size - 1)
		fail("nic_rx_size must return %d but %d", nic->rx.size - 1, size);

	if(!nic_has_rx(nic))
		fail("nic_has_rx must return true");

	pass();


	printf("rx queue: pop: ");
	for(i = 0; i < nic->rx.size - 1; i++) {
		Packet* p1 = nic_rx(nic);
		if(p1 != ps[i])
			fail("worong pointer returned: %p, expected: %p", p1, (void*)(uintptr_t)i);

		nic_free(p1);
	}

	if(!nic_driver_has_rx(nic))
		fail("nic_driver_has_rx must return true");

	size = nic_rx_size(nic);
	if(size != 0)
		fail("nic_rx_size must return 0 but %d", size);

	if(nic_has_rx(nic))
		fail("nic_has_rx must return false");

	pass();


	printf("rx queue: push one after overflow: ");
	p1 = nic_alloc(nic, 0);
	if(p1 == NULL)
		fail("cannot alloc packet: %p\n", p1);

	if(!nic_driver_rx2(nic, p1))
		fail("nic_driver_rx must be return true");

	if(!nic_has_rx(nic))
		fail("nic_has_rx must be true");

	size = nic_rx_size(nic);
	if(size != 1)
		fail("nic_rx_size must be 1: %d", size);

	pass();


	printf("rx queue: pop one after overflow: ");

	p2 = nic_rx(nic);
	if(p1 != p2)
		fail("nic_rx returned wrong pointer: %p != %p", p1, p2);

	size = nic_rx_size(nic);
	if(size != 0)
		fail("nic_rx_size must be 0: %d", size);

	nic_free(p2);

	used = bitmap_used(nic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);

	pass();


	printf("srx queue: Check initial status: ");
	if(nic_has_srx(nic))
		fail("nic_has_srx must be false");

	p1 = nic_srx(nic);
	if(p1 != NULL)
		fail("nic_srx must be NULL: %p", p1);

	size = nic_srx_size(nic);
	if(size != 0)
		fail("nic_srx_size must be 0: %d", size);

	if(!nic_driver_has_srx(nic))
		fail("nic_driver_has_srx must be true");

	pass();


	printf("srx queue: push one: ");
	p1 = nic_alloc(nic, 64);
	if(!nic_driver_srx2(nic, p1))
		fail("nic_driver_srx must be return true");

	if(!nic_has_srx(nic))
		fail("nic_has_srx must be true");

	size = nic_srx_size(nic);
	if(size != 1)
		fail("nic_srx_size must be 1: %d", size);

	pass();


	printf("srx queue: pop one: ");

	p2 = nic_srx(nic);
	if(p2 != p1)
		fail("nic_srx returned wrong pointer: %p != %p", p1, p2);

	size = nic_srx_size(nic);
	if(size != 0)
		fail("nic_srx_size must be 0: %d", size);

	nic_free(p2);

	pass();


	printf("srx queue: push full: ");
	for(i = 0; i < nic->srx.size - 1; i++) {
		ps[i] = nic_alloc(nic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i);

		if(!nic_driver_srx2(nic, ps[i]))
			fail("cannot push srx: count: %d, queue size: %d", i, nic->srx.size);
	}

	if(nic_driver_has_srx(nic))
		fail("nic_driver_has_srx must return false");

	size = nic_srx_size(nic);
	if(size != nic->srx.size - 1)
		fail("nic_srx_size must return %d but %d", nic->srx.size - 1, size);

	if(!nic_has_srx(nic))
		fail("nic_has_srx must return true");

	pass();


	printf("srx queue: overflow: ");

	used = bitmap_used(nic);

	p1 = nic_alloc(nic, 0);
	if(p1 == NULL)
		fail("packet allocation failed");

	if(nic_driver_srx2(nic, p1))
		fail("push overflow: count: %d, queue size: %d", i, nic->srx.size);

	used2 = bitmap_used(nic);
	if(used != used2)
		fail("packet pool usage is changed: used: %d != %d", used, used2);

	if(nic_driver_has_srx(nic))
		fail("nic_driver_has_srx must return false");

	size = nic_srx_size(nic);
	if(size != nic->srx.size - 1)
		fail("nic_srx_size must return %d but %d", nic->srx.size - 1, size);

	if(!nic_has_srx(nic))
		fail("nic_has_srx must return true");

	pass();


	printf("srx queue: pop: ");
	for(i = 0; i < nic->srx.size - 1; i++) {
		Packet* p1 = nic_srx(nic);
		if(p1 != ps[i])
			fail("worong pointer returned: %p, expected: %p", p1, (void*)(uintptr_t)i);

		nic_free(p1);
	}

	if(!nic_driver_has_srx(nic))
		fail("nic_driver_has_srx must return true");

	size = nic_srx_size(nic);
	if(size != 0)
		fail("nic_srx_size must return 0 but %d", size);

	if(nic_has_srx(nic))
		fail("nic_has_srx must return false");

	pass();


	printf("srx queue: push one after overflow: ");
	p1 = nic_alloc(nic, 0);
	if(p1 == NULL)
		fail("cannot alloc packet: %p\n", p1);

	if(!nic_driver_srx2(nic, p1))
		fail("nic_driver_srx must be return true");

	if(!nic_has_srx(nic))
		fail("nic_has_srx must be true");

	size = nic_srx_size(nic);
	if(size != 1)
		fail("nic_srx_size must be 1: %d", size);

	pass();


	printf("srx queue: pop one after overflow: ");

	p2 = nic_srx(nic);
	if(p1 != p2)
		fail("nic_srx returned wrong pointer: %p != %p", p1, p2);

	size = nic_srx_size(nic);
	if(size != 0)
		fail("nic_srx_size must be 0: %d", size);

	nic_free(p2);

	used = bitmap_used(nic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);

	pass();



	printf("tx queue: Check initial state: ");

	size = nic_tx_size(nic);
	if(size != 0)
		fail("nic_tx_size must be 0: %d", size);

	if(!nic_has_tx(nic))
		fail("nic_has_tx must be true");

	if(nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must false");

	pass();


	printf("tx queue: tx one: ");
	p1 = nic_alloc(nic, 0);

	if(!nic_tx(nic, p1))
		fail("nic_tx must be true");

	size = nic_tx_size(nic);
	if(size != 1)
		fail("nic_tx_size must be 1: %d", size);

	if(!nic_has_tx(nic))
		fail("nic_has_tx must be true");

	if(!nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must true");

	pass();


	printf("tx queue: send one: ");
	p1 = nic_driver_tx(nic);
	nic_free(p1);

	used = bitmap_used(nic);
	if(used != 0)
		fail("cannot free packet: %d", used);

	size = nic_tx_size(nic);
	if(size != 0)
		fail("nic_tx_size must be 0: %d", size);

	if(!nic_has_tx(nic))
		fail("nic_has_tx must be true");

	if(nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must false");

	pass();


	printf("tx queue: tx full: ");
	for(i = 0; i < nic->tx.size - 1; i++) {
		ps[i] = nic_alloc(nic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i + 1);

		if(!nic_tx(nic, ps[i]))
			fail("cannot tx packet: index: %d", i);
	}

	size = nic_tx_size(nic);
	if(size != nic->tx.size - 1)
		fail("nic_tx_size must be %d: %d", nic->tx.size - 1, size);

	if(nic_has_tx(nic))
		fail("nic_has_tx must be false");

	if(!nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must true");

	pass();


	printf("tx queue: overflow: ");

	used = bitmap_used(nic);

	p1 = nic_alloc(nic, 0);

	if(nic_tx(nic, p1))
		fail("nic_tx overflow");

	used2 = bitmap_used(nic);

	if(used != used2)
		fail("packet not freed on overflow %d != %d", used, used2);

	if(nic_try_tx(nic, p1))
		fail("nic_try_tx overflow");

	used2 = bitmap_used(nic);

	if(used != used2)
		fail("packet freed on try_tx %d != %d", used, used2);

	if(nic_tx_dup(nic, p1))
		fail("nic_tx_dup overflow");

	used2 = bitmap_used(nic);

	if(used != used2)
		fail("packet allocated on overflow %d != %d", used, used2);

	size = nic_tx_size(nic);
	if(size != nic->tx.size - 1)
		fail("nic_tx_size must be %d: %d", nic->tx.size - 1, size);

	if(nic_has_tx(nic))
		fail("nic_has_tx must be false");

	if(!nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must true");

	pass();


	printf("tx queue: send all: ");
	for(i = 0; i < nic->tx.size - 1; i++) {
		p1 = nic_driver_tx(nic);
		if(p1 != ps[i])
			fail("wrong pointer returned: %p != %p", ps[i], p1);

		if(!nic_free(p1))
			fail("cannot free packet: %p", p1);
	}

	size = nic_tx_size(nic);
	if(size != 0)
		fail("nic_tx_size must be 0: %d", 0, size);

	if(!nic_has_tx(nic))
		fail("nic_has_tx must be true");

	if(nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must false");

	used = bitmap_used(nic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);

	pass();


	printf("tx queue: tx one after overflow: ");
	p1 = nic_alloc(nic, 0);

	if(!nic_tx(nic, p1))
		fail("nic_tx must be true");

	size = nic_tx_size(nic);
	if(size != 1)
		fail("nic_tx_size must be 1: %d", size);

	if(!nic_has_tx(nic))
		fail("nic_has_tx must be true");

	if(!nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must true");

	pass();


	printf("tx queue: send one after overflow: ");
	p1 = nic_driver_tx(nic);
	nic_free(p1);

	used = bitmap_used(nic);
	if(used != 0)
		fail("cannot free packet: %d", used);

	size = nic_tx_size(nic);
	if(size != 0)
		fail("nic_tx_size must be 0: %d", size);

	if(!nic_has_tx(nic))
		fail("nic_has_tx must be true");

	if(nic_driver_has_tx(nic))
		fail("nic_driver_has_tx must false");

	pass();


	printf("Stx queue: Check initial state: ");

	size = nic_stx_size(nic);
	if(size != 0)
		fail("nic_stx_size must be 0: %d", size);

	if(!nic_has_stx(nic))
		fail("nic_has_stx must be true");

	if(nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must false");

	pass();


	printf("Stx queue: stx one: ");
	p1 = nic_alloc(nic, 0);

	if(!nic_stx(nic, p1))
		fail("nic_stx must be true");

	size = nic_stx_size(nic);
	if(size != 1)
		fail("nic_stx_size must be 1: %d", size);

	if(!nic_has_stx(nic))
		fail("nic_has_stx must be true");

	if(!nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must true");

	pass();


	printf("Stx queue: send one: ");
	p1 = nic_driver_stx(nic);
	nic_free(p1);

	used = bitmap_used(nic);
	if(used != 0)
		fail("cannot free packet: %d", used);

	size = nic_stx_size(nic);
	if(size != 0)
		fail("nic_stx_size must be 0: %d", size);

	if(!nic_has_stx(nic))
		fail("nic_has_stx must be true");

	if(nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must false");

	pass();


	printf("Stx queue: stx full: ");
	for(i = 0; i < nic->stx.size - 1; i++) {
		ps[i] = nic_alloc(nic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i + 1);

		if(!nic_stx(nic, ps[i]))
			fail("cannot stx packet: index: %d", i);
	}

	size = nic_stx_size(nic);
	if(size != nic->stx.size - 1)
		fail("nic_stx_size must be %d: %d", nic->stx.size - 1, size);

	if(nic_has_stx(nic))
		fail("nic_has_stx must be false");

	if(!nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must true");

	pass();


	printf("Stx queue: overflow: ");

	used = bitmap_used(nic);

	p1 = nic_alloc(nic, 0);

	if(nic_stx(nic, p1))
		fail("nic_stx overflow");

	used2 = bitmap_used(nic);

	if(used != used2)
		fail("packet not freed on overflow %d != %d", used, used2);

	if(nic_try_stx(nic, p1))
		fail("nic_try_stx overflow");

	used2 = bitmap_used(nic);

	if(used != used2)
		fail("packet freed on try_stx %d != %d", used, used2);

	if(nic_stx_dup(nic, p1))
		fail("nic_stx_dup overflow");

	used2 = bitmap_used(nic);

	if(used != used2)
		fail("packet allocated on overflow %d != %d", used, used2);

	size = nic_stx_size(nic);
	if(size != nic->stx.size - 1)
		fail("nic_stx_size must be %d: %d", nic->stx.size - 1, size);

	if(nic_has_stx(nic))
		fail("nic_has_stx must be false");

	if(!nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must true");

	pass();


	printf("Stx queue: send all: ");
	for(i = 0; i < nic->stx.size - 1; i++) {
		p1 = nic_driver_stx(nic);
		if(p1 != ps[i])
			fail("wrong pointer returned: %p != %p", ps[i], p1);

		if(!nic_free(p1))
			fail("cannot free packet: %p", p1);
	}

	size = nic_stx_size(nic);
	if(size != 0)
		fail("nic_stx_size must be 0: %d", 0, size);

	if(!nic_has_stx(nic))
		fail("nic_has_stx must be true");

	if(nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must false");

	used = bitmap_used(nic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);

	pass();


	printf("Stx queue: stx one after overflow: ");
	p1 = nic_alloc(nic, 0);

	if(!nic_stx(nic, p1))
		fail("nic_stx must be true");

	size = nic_stx_size(nic);
	if(size != 1)
		fail("nic_stx_size must be 1: %d", size);

	if(!nic_has_stx(nic))
		fail("nic_has_stx must be true");

	if(!nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must true");

	pass();


	printf("Stx queue: send one after overflow: ");
	p1 = nic_driver_stx(nic);
	nic_free(p1);

	used = bitmap_used(nic);
	if(used != 0)
		fail("cannot free packet: %d", used);

	size = nic_stx_size(nic);
	if(size != 0)
		fail("nic_stx_size must be 0: %d", size);

	if(!nic_has_stx(nic))
		fail("nic_has_stx must be true");

	if(nic_driver_has_stx(nic))
		fail("nic_driver_has_stx must false");

	pass();


	printf("Config: register one: ");
	uint32_t key = nic_config_register(nic, "config");
	if(key == 0)
		fail("cannot register config: %u", key);

	pass();


	printf("Config: key: ");
	uint32_t key2 = nic_config_key(nic, "config");
	if(key != key2)
		fail("cannot get key: %u != %u", key, key2);

	pass();


	printf("Config: put: ");
	if(!nic_config_put(nic, key, 0x1234567890abcdef))
		fail("cannot put config");

	pass();


	printf("Config: put with illegal key: ");
	if(nic_config_put(nic, key + 1, 0x1122334455667788))
		fail("illegal key put: %d", key + 1);

	pass();

	printf("Config: get: ");
	uint64_t config = nic_config_get(nic, key);
	if(config != 0x1234567890abcdef)
		fail("config returns wrong value: %lx != %lx", config, 0x1234567890abcdef);

	pass();


	printf("Config: put full: ");
	uint32_t available = (uint32_t)((uintptr_t)nic->config_tail - (uintptr_t)nic->config_head);
	available -= 1 + 7 + 8;

	i = 1;
	char name[9] = { 0, };
	while(available > 1 + 9 + 8) {
		sprintf(name, "%08x", i);
		key = nic_config_register(nic, name);
		if(key == 0)
			fail("cannot register config: '%s'", name);

		if(!nic_config_put(nic, key, i))
			fail("cannot put config: %s %d", name, i);

		available -= 1 + 9 + 8;
		i++;
	}
	printf("available: %d", available);

	pass();


	printf("Config: overflow: ");
	key = nic_config_register(nic, "hello");
	if(key != 0)
		fail("overflow: %u", key);

	pass();


	printf("Config: get full: ");
	for(int j = 1; j < i; j++) {
		sprintf(name, "%08x", j);
		key = nic_config_key(nic, name);
		if(key == 0)
			fail("cannot get key: '%s'", name);

		uint64_t value = nic_config_get(nic, key);

		if(value != j)
			fail("wrong config value returned: %ld !=  %ld", i, j);
	}

	pass();

	return 0;
}

#endif /* TEST */
