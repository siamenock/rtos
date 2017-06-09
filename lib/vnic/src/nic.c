#include <string.h>
#include <strings.h>
#include "lock.h"
#include "nic.h"

NIC* __nics[NIC_MAX_COUNT];
int __nic_count;

NIC* nic_find_by_packet(Packet* packet) {
	NIC* nic = (void*)((uintptr_t)packet & ~(uintptr_t)(0x200000 - 1)); // 2MB alignment
	for(int i = 0; i < NIC_MAX_SIZE / 0x200000  - 1 && (uintptr_t)nic > 0; i++) {
		if(nic->magic == NIC_MAGIC_HEADER)
			return nic;

		nic = (void*)nic - 0x200000;
	}

	return NULL;
}

int nic_count() {
	return __nic_count;
}

NIC* nic_get(int index) {
	return index < __nic_count ? __nics[index] : NULL;
}

NIC* nic_get_by_id(uint32_t id) {
	NIC* nic = NULL;
	for(int i = 0; i < __nic_count; i++) {
		if(__nics[i] && __nics[i]->id == id) {
			nic =  __nics[i];
			break;
		}
	}
	return nic;
}

Packet* nic_alloc(NIC* nic, uint16_t size) {
	uint8_t* bitmap = (void*)nic + nic->pool.bitmap;
	uint32_t count = nic->pool.count;
	void* pool = (void*)nic + nic->pool.pool;

	uint32_t size2 = sizeof(Packet) + nic->padding_head + size + nic->padding_tail;
	uint8_t req = (ROUNDUP(size2, NIC_CHUNK_SIZE)) / NIC_CHUNK_SIZE;
	uint32_t index = nic->pool.index;

	lock_lock(&nic->pool.lock);

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
	NIC* nic = nic_find_by_packet(packet);
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

	lock_lock(&nic->pool.lock);
	nic->pool.used -= req;
	lock_unlock(&nic->pool.lock);

	return true;
}

bool queue_push(NIC* nic, NICQueue* queue, Packet* packet) {
	NIC* nic2 = nic_find_by_packet(packet);
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

void* queue_pop(NIC* nic, NICQueue* queue) {
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

uint32_t queue_size(NICQueue* queue) {
	if(queue->tail >= queue->head)
		return queue->tail - queue->head;
	else
		return queue->size + queue->tail - queue->head;
}

bool queue_available(NICQueue* queue) {
	return queue->head != (queue->tail + 1) % queue->size;
}

bool queue_empty(NICQueue* queue) {
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

bool nic_has_tx(NIC* nic) {
	return !queue_empty(&nic->tx);
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

bool nic_tx_available(NIC* nic) {
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

/**
 * Payload
 * name_length: uint16_t
 * block_count: uint16_t
 * name: 4 bytes rounded string length
 * blocks
 * @return -1 key length is too long
 * @return -2 already allocated
 * @return -3 no space to allocate
 * @return 0 ~ 2^16 - 1 key
 */
int32_t nic_config_alloc(NIC* nic, char* name, uint16_t size) {
	int len = strlen(name) + 1;
	if(len > 255)
		return -1;
	
	uint32_t req = 1 + ((len + sizeof(uint32_t) - 1) / sizeof(uint32_t)) + (((uint32_t)size + sizeof(uint32_t) - 1) / sizeof(uint32_t)); // heder + round(name) + round(blocks)
	
	// Check name is already allocated
	for(uint32_t* p = (uint32_t*)nic->config_head; p < (uint32_t*)nic->config_tail; ) {
		uint16_t len2 = *p >> 16;
		uint16_t count2 = *p & 0xffff;
		
		if(*p == 0) {
			p++;
		} else {
			if(len2 == len && strncmp(name, (const char*)(p + 1), len - 1) == 0) {
				return -1;
			}
			
			p += count2;
		}
	}
	
	// Check available space
	for(uint32_t* p = (uint32_t*)nic->config_head; p < (uint32_t*)nic->config_tail; ) {
		if(*p == 0) {
			bool found = true;
			for(int i = 0; i < req; i++) {
				if(p >= (uint32_t*)nic->config_tail) {
					return -2;
				}
				
				if(p[i] != 0) {
					p += i - 1;
					found = false;
					break;
				}
			}
			
			if(found) {
				*p = (uint32_t)len << 16 | req;
				memcpy(p + 1, name, len);
				return ((uintptr_t)p - (uintptr_t)nic->config_head) / sizeof(uint32_t);
			}
			
			p++;
		} else {
			p += *p & 0xffff;
		}
	}
	
	return -2;
}

void nic_config_free(NIC* nic, uint16_t key) {
	uint16_t count = (uint16_t)nic->config_head[key] & 0xffff;
	
	for(int i = count - 1; i >= 0; i--) {
		nic->config_head[key + i] = 0;
	}
}

/**
 *
 * @return -1 key length is too long
 * @return -2 key of the name not found
 * @return otherwise key of the name
 */
int32_t nic_config_key(NIC* nic, char* name) {
	int len = strlen(name) + 1;
	if(len > 255)
		return -1;
	
	for(uint32_t* p = (uint32_t*)nic->config_head; p < (uint32_t*)nic->config_tail; ) {
		if(*p == 0) {
			p++;
		} else {
			uint16_t len2 = *p >> 16;
			uint16_t count2 = *p & 0xffff;
			
			if(len2 == len && strncmp(name, (const char*)(p + 1), len - 1) == 0) {
				return ((uintptr_t)p - (uintptr_t)nic->config_head) / sizeof(uint32_t);
			}
			
			p += count2;
		}
	}

	return -2;
}

void* nic_config_get(NIC* nic, uint16_t key) {
	uint32_t* header = (uint32_t*)&nic->config_head[key];
	uint16_t len = *header >> 16;
	
	return header + 1 + (len + sizeof(uint32_t) - 1) / sizeof(uint32_t);
}

uint16_t nic_config_size(NIC* nic, uint16_t key) {
	uint32_t* header = (uint32_t*)&nic->config_head[key];
	uint16_t len = *header >> 16;
	uint16_t count = *header & 0xffff;
	
	return (count - (len + sizeof(uint32_t) - 1) / sizeof(uint32_t) - 1) * sizeof(uint32_t);
}

uint32_t nic_config_available(NIC* nic) {
	uint32_t count = 0;
	
	for(uint32_t* p = (uint32_t*)nic->config_head; p < (uint32_t*)nic->config_tail; ) {
		if(*p == 0) {
			p++;
			count++;
		} else {
			p += *p & 0xffff;
		}
	}

	return count * sizeof(uint32_t);
}

uint32_t nic_config_total(NIC* nic) {
	return (uint32_t)((uintptr_t)nic->config_tail - (uintptr_t)nic->config_head);
}

#if TEST

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void print_queue(NICQueue* queue) {
	printf("\tbase: %d\n", queue->base);
	printf("\thead: %d\n", queue->head);
	printf("\ttail: %d\n", queue->tail);
	printf("\tsize: %d\n", queue->size);
}

static void print_config(VNIC* vnic) {	
	for(uint32_t* p = vnic->config_head; p < vnic->config_tail; ) {
		if(*p == 0) {
			p++;
		} else {
			uint16_t len2 = *p >> 16;
			uint16_t count2 = *p & 0xffff;
			
			printf("[%3d] %3d %3d \"%s\" ", (int)(((uintptr_t)p - (uintptr_t)vnic->config_head) / sizeof(uint32_t)), len2, count2, (char*)(p + 1));
			uint32_t* base = p + 1 + (len2 + 3) / 4;
			uint16_t count3 = count2 - 1 - (len2 + 3) / 4;
			for(int i = 0; i < count3 && i < 12; i++) {
				printf("%08x ", base[i]);
			}
			if(count3 >= 12)
				printf("...\n");
			else
				printf("\n");
			
			p += count2;
		}
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

static void dump_queue(NIC* nic, NICQueue* queue) {
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
	fprintf(stderr, "\tFAILED\n"
	
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, arg
	va_end(argptr);

	printf("\n");
	
	VNIC* vnic = (VNIC*)buffer;
	
	/*
	printf("* rx\n");
	dump_queue(vnic, &vnic->rx);
	
	printf("* tx\n");
	dump_queue(vnic, &vnic->tx);
	
	printf("* srx\n");
	dump_queue(vnic, &vnic->srx)
	
	printf("* stx\n");
	dump_queue(vnic, &vnic->stx)
	
	printf("* Bitmap\n");
	dump_bitmap(vnic);
	*/
	printf("* Config\n");
	print_config(vnic);
	
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
	

	uint16_t total = vnic_config_total(vnic);
	uint16_t available = vnic_config_available(vnic);

	printf("Config: Check total and available size equals: ");
	if(total != available)
		fail("available: %d != total: %d\n", available, total);
	
	pass();
	
	printf("Config: alloc one: ");
	int32_t key = vnic_config_alloc(vnic, "net.ipv4", 12);
	if(key < 0)
		fail("cannot alloc config: %d", key);
	else
		printf("%d", key);
	
	pass();
	
	printf("Config: available consumption check 1: ");
	uint32_t available2 = vnic_config_available(vnic);
	if(available2 != available - 4 - 12 - 12)
		fail("available memory is differ: %d expected: %d\n", available2, available - 4 - 12 - 12);
	
	available += - 4 - 12 - 12;
	
	pass();
	
	printf("Config: find key: ");
	int32_t key2 = vnic_config_key(vnic, "net.ipv4");
	if(key2 != key)
		fail("cannot find key for net.ipv4: %d", key2);
	
	pass();
	
	printf("Config: get size: ");
	size = vnic_config_size(vnic, key);
	if(size != 12)
		fail("config size is wrong: %d\n", size);
	else
		printf("%d", size);
	
	pass();
	
	
	printf("Config: alloc next: ");
	key2 = vnic_config_alloc(vnic, "net.ipv6", 48);
	if(key2 < 0)
		fail("cannot alloc config: %d", key2);
	
	if(key + 1 + (strlen("net.ipv4") + 1 + 3) / 4 + 12 / 4 != key2)
		fail("wrong second key allocated: %d", key2);
	
	pass();
	
	printf("Config: available consumption check 2: ");
	available2 = vnic_config_available(vnic);
	if(available2 != available - 4 - 12 - 48)
		fail("available memory is differ: %d expected: %d\n", available2, available - 4 - 12 - 48);
	
	available += - 4 - 12 - 48;
	
	pass();
	
	printf("Config: find second key: ");
	int32_t key3 = vnic_config_key(vnic, "net.ipv6");
	if(key3 != key2)
		fail("cannot find key for net.ipv6: %d", key3);
	
	pass();
	
	printf("Config: get size: ");
	size = vnic_config_size(vnic, key2);
	if(size != 48)
		fail("config size is wrong: %d\n", size);
	else
		printf("%d", size);
	
	pass();
	
	printf("Config: check overwriting: ");
	
	uint32_t* ipv4 = vnic_config_get(vnic, key);
	ipv4[0] = 0x11223344;
	ipv4[1] = 0x55667788;
	ipv4[2] = 0x99001122;
	
	uint64_t* ipv6 = vnic_config_get(vnic, key2);
	ipv6[0] = 0x0102030405060708;
	ipv6[1] = 0x0900010203040506;
	ipv6[2] = 0x0708091011121314;
	ipv6[3] = 0x1516171819202122;
	ipv6[4] = 0x2324252627282930;
	ipv6[5] = 0x3132333435363738;
	
	printf("Config: find key(2): ");
	key3 = vnic_config_key(vnic, "net.ipv4");
	if(key3 != key)
		fail("cannot find key for net.ipv4: %d", key3);
	
	pass();
	
	printf("Config: get size(2): ");
	size = vnic_config_size(vnic, key);
	if(size != 12)
		fail("config size is wrong: %d\n", size);
	else
		printf("%d", size);
	
	pass();
	
	
	printf("Config: find second key(2): ");
	key3 = vnic_config_key(vnic, "net.ipv6");
	if(key3 != key2)
		fail("cannot find key for net.ipv6: %d", key3);
	
	pass();
	
	printf("Config: get size(2): ");
	size = vnic_config_size(vnic, key2);
	if(size != 48)
		fail("config size is wrong: %d\n", size);
	else
		printf("%d", size);
	
	pass();
	

	printf("Config: free first alloc: ");
	vnic_config_free(vnic, key);
	
	key = vnic_config_key(vnic, "net.ipvs");
	if(key >= 0)
		fail("freed key exists: %d", key);
	
	pass();
	
	printf("Config: available consumption check 3: ");
	available2 = vnic_config_available(vnic);
	if(available2 != available + 4 + 12 + 12)
		fail("available memory is differ: %d expected: %d\n", available2, available + 4 + 12 + 12);
	
	available += 4 + 12 + 12;
	
	printf("Config: overflow: ");
	uint32_t chunk = available - 4 - 12 - 12 - 4 - 8;
	key3 = vnic_config_alloc(vnic, "chunk", chunk);
	if(key3 < 0)
		fail("cannot allocate big chunk: %d", key3);
	
	key = vnic_config_alloc(vnic, "net.ipv4", 12);
	if(key < 0)
		fail("cannot allocate first chunk: %d", key);
	
	pass();
	
	printf("Config: available consumption check 4: ");
	available = vnic_config_available(vnic);
	if(available != 0)
		fail("available memory is differ: %d expected: %d\n", available, 0);
	
	pass();
	
	print_config(vnic);
	
	return 0;
}

#endif /* TEST */
