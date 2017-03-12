#include <string.h>
#include <strings.h>
#include "lock.h"
#include "vnic.h"

#define ALIGN(x, y)	((((x) + (y) - 1) / (y)) * (y))

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
	uint8_t req = (ALIGN(size2, VNIC_CHUNK_SIZE)) / VNIC_CHUNK_SIZE;

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

bool vnic_has_input(VNIC* vnic) {
	return !queue_empty(&vnic->input);
}

Packet* vnic_input(VNIC* vnic) {
	lock_lock(&vnic->input.rlock);
	Packet* packet = queue_pop(vnic, &vnic->input);
	lock_unlock(&vnic->input.rlock);
	
	return packet;
}

uint32_t vnic_input_size(VNIC* vnic) {
	return queue_size(&vnic->input);
}

bool vnic_has_slow_input(VNIC* vnic) {
	return !queue_empty(&vnic->slow_input);
}

Packet* vnic_slow_input(VNIC* vnic) {
	lock_lock(&vnic->slow_input.rlock);
	Packet* packet = queue_pop(vnic, &vnic->slow_input);
	lock_unlock(&vnic->slow_input.rlock);

	return packet;
}

uint32_t vnic_slow_input_size(VNIC* vnic) {
	return queue_size(&vnic->slow_input);
}

bool vnic_output(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->output.wlock);
	if(!queue_push(vnic, &vnic->output, packet)) {
		lock_unlock(&vnic->output.wlock);
		
		vnic_free(packet);
		return false;
	} else {
		lock_unlock(&vnic->output.wlock);
		return true;
	}
}

bool vnic_output_try(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->output.wlock);
	bool result = queue_push(vnic, &vnic->output, packet);
	lock_unlock(&vnic->output.wlock);
	
	return result;
}

bool vnic_output_dup(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->output.wlock);
	if(!queue_available(&vnic->output)) {
		lock_unlock(&vnic->output.wlock);
		return false;
	}
	
	int len = packet->end - packet->start;
	
	Packet* packet2 = vnic_alloc(vnic, len);
	if(!packet2) {
		lock_unlock(&vnic->output.wlock);
		return false;
	}
	
	packet2->time = packet->time;
	packet2->end = packet2->start + len;
	memcpy(packet2->buffer + packet2->start, packet->buffer + packet->start, len);
	
	if(!queue_push(vnic, &vnic->output, packet)) {
		lock_unlock(&vnic->output.wlock);
		
		vnic_free(packet);
		return false;
	} else {
		lock_unlock(&vnic->output.wlock);
		return true;
	}
}

bool vnic_output_available(VNIC* vnic) {
	return queue_available(&vnic->output);
}

uint32_t vnic_output_size(VNIC* vnic) {
	return queue_size(&vnic->output);
}

bool vnic_slow_output(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->slow_output.wlock);
	if(!queue_push(vnic, &vnic->slow_output, packet)) {
		lock_unlock(&vnic->slow_output.wlock);
		vnic_free(packet);
		
		return false;
	} else {
		lock_unlock(&vnic->slow_output.wlock);
		return true;
	}
}

bool vnic_slow_output_try(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->slow_output.wlock);
	bool result = queue_push(vnic, &vnic->slow_output, packet);
	lock_unlock(&vnic->slow_output.wlock);

	return result;
}

bool vnic_slow_output_dup(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->slow_output.wlock);
	if(!queue_available(&vnic->slow_output)) {
		lock_unlock(&vnic->slow_output.wlock);
		return false;
	}
	
	int len = packet->end - packet->start;
	
	Packet* packet2 = vnic_alloc(vnic, len);
	if(!packet2) {
		lock_unlock(&vnic->slow_output.wlock);
		return false;
	}
	
	packet2->time = packet->time;
	packet2->end = packet2->start + len;
	memcpy(packet2->buffer + packet2->start, packet->buffer + packet->start, len);
	
	if(!queue_push(vnic, &vnic->slow_output, packet)) {
		lock_unlock(&vnic->slow_output.wlock);
		vnic_free(packet);
		
		return false;
	} else {
		lock_unlock(&vnic->slow_output.wlock);
		return true;
	}
}

bool vnic_slow_output_available(VNIC* vnic) {
	return queue_available(&vnic->slow_output);
}

uint32_t vnic_slow_output_size(VNIC* vnic) {
	return queue_size(&vnic->slow_output);
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
void vnic_init(uint32_t id, uint64_t mac, void* base, size_t size, 
		uint64_t input_bandwidth, uint64_t output_bandwidth, 
		uint16_t padding_head, uint16_t padding_tail, 
		uint32_t input_queue_size, uint32_t output_queue_size, 
		uint32_t slow_input_queue_size, uint32_t slow_output_queue_size) {
	
	int index = sizeof(VNIC);
	
	VNIC* vnic = base;
	vnic->magic = VNIC_MAGIC_HEADER;
	vnic->id = id;
	vnic->mac = mac;
	vnic->input_bandwidth = input_bandwidth;
	vnic->output_bandwidth = output_bandwidth;
	vnic->padding_head = padding_head;
	vnic->padding_tail = padding_tail;
	
	vnic->input.base = index;
	vnic->input.head = 0;
	vnic->input.tail = 0;
	vnic->input.size = input_queue_size;
	vnic->input.rlock = 0;
	vnic->input.wlock = 0;
	
	index += vnic->input.size * sizeof(uint64_t);
	index = ALIGN(index, 8);
	vnic->output.base = index;
	vnic->output.head = 0;
	vnic->output.tail = 0;
	vnic->output.size = output_queue_size;
	vnic->output.rlock = 0;
	vnic->output.wlock = 0;
	
	index += vnic->output.size * sizeof(uint64_t);
	index = ALIGN(index, 8);
	vnic->slow_input.base = index;
	vnic->slow_input.head = 0;
	vnic->slow_input.tail = 0;
	vnic->slow_input.size = input_queue_size;
	vnic->slow_input.rlock = 0;
	vnic->slow_input.wlock = 0;
	
	index += vnic->slow_input.size * sizeof(uint64_t);
	index = ALIGN(index, 8);
	vnic->slow_output.base = index;
	vnic->slow_output.head = 0;
	vnic->slow_output.tail = 0;
	vnic->slow_output.size = output_queue_size;
	vnic->slow_output.rlock = 0;
	vnic->slow_output.wlock = 0;
	
	index += vnic->slow_output.size * sizeof(uint64_t);
	index = ALIGN(index, 8);
	vnic->pool.bitmap = index;
	index += (size - index) / VNIC_CHUNK_SIZE;
	index = ALIGN(index, VNIC_CHUNK_SIZE);
	vnic->pool.pool = index;
	vnic->pool.count = (size - index) / VNIC_CHUNK_SIZE;
	vnic->pool.index = 0;
	vnic->pool.used = 0;
	vnic->pool.lock = 0;
	
	vnic->config = 0;
	bzero(vnic->config_head, (size_t)((uintptr_t)vnic->config_tail - (uintptr_t)vnic->config_head));
	
	bzero(base + vnic->pool.bitmap, vnic->pool.count);
}

bool vnic_receivable(VNIC* vnic) {
	return queue_available(&vnic->input);
}

bool vnic_received(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&vnic->input.wlock);
	if(queue_available(&vnic->input)) {
		Packet* packet = vnic_alloc(vnic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&vnic->input.wlock);
			return false;
		}
		
		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);
		
		packet->end = packet->start + size1 + size2;
		
		if(queue_push(vnic, &vnic->input, packet)) {
			lock_unlock(&vnic->input.wlock);
			return true;
		} else {
			lock_unlock(&vnic->input.wlock);
			vnic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&vnic->input.wlock);
		return false;
	}
}

bool vnic_received2(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->input.wlock);
	if(queue_push(vnic, &vnic->input, packet)) {
		lock_unlock(&vnic->input.wlock);
		return true;
	} else {
		lock_unlock(&vnic->input.wlock);
		vnic_free(packet);
		return false;
	}
}

bool vnic_slow_receivable(VNIC* vnic) {
	return queue_available(&vnic->slow_input);
}

bool vnic_slow_received(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&vnic->slow_input.wlock);
	if(queue_available(&vnic->slow_input)) {
		Packet* packet = vnic_alloc(vnic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&vnic->slow_input.wlock);
			return false;
		}
		
		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);
		
		packet->end = packet->start + size1 + size2;
		
		if(queue_push(vnic, &vnic->slow_input, packet)) {
			lock_unlock(&vnic->slow_input.wlock);
			return true;
		} else {
			lock_unlock(&vnic->slow_input.wlock);
			vnic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&vnic->slow_input.wlock);
		return false;
	}
}

bool vnic_slow_received2(VNIC* vnic, Packet* packet) {
	lock_lock(&vnic->slow_input.wlock);
	if(queue_push(vnic, &vnic->slow_input, packet)) {
		lock_unlock(&vnic->slow_input.wlock);
		return true;
	} else {
		lock_unlock(&vnic->slow_input.wlock);
		vnic_free(packet);
		return false;
	}
}

bool vnic_sendable(VNIC* vnic) {
	return !queue_empty(&vnic->output);
}

Packet* vnic_send(VNIC* vnic) {
	lock_lock(&vnic->output.rlock);
	Packet* packet = queue_pop(vnic, &vnic->output);
	lock_unlock(&vnic->output.rlock);

	return packet;
}

bool vnic_slow_sendable(VNIC* vnic) {
	return !queue_empty(&vnic->slow_output);
}

Packet* vnic_slow_send(VNIC* vnic) {
	lock_lock(&vnic->slow_output.rlock);
	Packet* packet = queue_pop(vnic, &vnic->slow_output);
	lock_unlock(&vnic->slow_output.rlock);

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
	printf("input_bandwidth: %ld\n", vnic->input_bandwidth);
	printf("output_bandwidth: %ld\n", vnic->output_bandwidth);
	printf("padding_head: %d\n", vnic->padding_head);
	printf("padding_tail: %d\n", vnic->padding_tail);
	printf("input queue\n");
	print_queue(&vnic->input);
	printf("output queue\n");
	print_queue(&vnic->output);
	printf("slow_input queue\n");
	print_queue(&vnic->slow_input);
	printf("output slow_queue\n");
	print_queue(&vnic->slow_output);
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
	
	printf("* Input\n");
	dump_queue(vnic, &vnic->input);
	
	printf("* Output\n");
	dump_queue(vnic, &vnic->output);
	
	printf("* Slow input\n");
	dump_queue(vnic, &vnic->slow_input);
	
	printf("* Slow output\n");
	dump_queue(vnic, &vnic->slow_output);
	
	/*
	printf("* Bitmap\n");
	dump_bitmap(vnic);
	print_config(vnic);
	*/
	
	exit(1);
}

int main(int argc, char** argv) {
	VNIC* vnic = (VNIC*)buffer;
	
	vnic_init(1, 0x001122334455, vnic, 2 * 1024 * 1024,
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
	
	
	printf("Input queue: Check initial status: ");
	if(vnic_has_input(vnic))
		fail("vnic_has_input must be false");
	
	p1 = vnic_input(vnic);
	if(p1 != NULL)
		fail("vnic_input must be NULL: %p", p1);

	uint32_t size = vnic_input_size(vnic);
	if(size != 0)
		fail("vnic_input_size must be 0: %d", size);
	
	if(!vnic_receivable(vnic))
		fail("vnic_receivable must be true");
	
	pass();


	printf("Input queue: push one: ");
	p1 = vnic_alloc(vnic, 64);
	if(!vnic_received2(vnic, p1))
		fail("vnic_received must be return true");
	
	if(!vnic_has_input(vnic))
		fail("vnic_has_input must be true");
	
	size = vnic_input_size(vnic);
	if(size != 1)
		fail("vnic_input_size must be 1: %d", size);
	
	pass();
	

	printf("Input queue: pop one: ");
	
	Packet* p2 = vnic_input(vnic);
	if(p2 != p1)
		fail("vnic_input returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_input_size(vnic);
	if(size != 0)
		fail("vnic_input_size must be 0: %d", size);
	
	vnic_free(p2);
	
	pass();
	

	printf("Input queue: push full: ");
	for(i = 0; i < vnic->input.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i);
		
		if(!vnic_received2(vnic, ps[i]))
			fail("cannot push input: count: %d, queue size: %d", i, vnic->input.size);
	}
	
	if(vnic_receivable(vnic))
		fail("vnic_receivable must return false");
	
	size = vnic_input_size(vnic);
	if(size != vnic->input.size - 1)
		fail("vnic_input_size must return %d but %d", vnic->input.size - 1, size);
	
	if(!vnic_has_input(vnic))
		fail("vnic_has_input must return true");
	
	pass();
	

	printf("Input queue: overflow: ");

	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("packet allocation failed");
	
	if(vnic_received2(vnic, p1))
		fail("push overflow: count: %d, queue size: %d", i, vnic->input.size);
	
	int used2 = bitmap_used(vnic);
	if(used != used2)
		fail("packet pool usage is changed: used: %d != %d", used, used2);
	
	if(vnic_receivable(vnic))
		fail("vnic_receivable must return false");
	
	size = vnic_input_size(vnic);
	if(size != vnic->input.size - 1)
		fail("vnic_input_size must return %d but %d", vnic->input.size - 1, size);
	
	if(!vnic_has_input(vnic))
		fail("vnic_has_input must return true");
	
	pass();
	

	printf("Input queue: pop: ");
	for(i = 0; i < vnic->input.size - 1; i++) {
		Packet* p1 = vnic_input(vnic);
		if(p1 != ps[i])
			fail("worong pointer returned: %p, expected: %p", p1, (void*)(uintptr_t)i);

		vnic_free(p1);
	}
	
	if(!vnic_receivable(vnic))
		fail("vnic_receivable must return true");
	
	size = vnic_input_size(vnic);
	if(size != 0)
		fail("vnic_input_size must return 0 but %d", size);
	
	if(vnic_has_input(vnic))
		fail("vnic_has_input must return false");
	
	pass();


	printf("Input queue: push one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("cannot alloc packet: %p\n", p1);
	
	if(!vnic_received2(vnic, p1))
		fail("vnic_received must be return true");
	
	if(!vnic_has_input(vnic))
		fail("vnic_has_input must be true");
	
	size = vnic_input_size(vnic);
	if(size != 1)
		fail("vnic_input_size must be 1: %d", size);
	
	pass();
	

	printf("Input queue: pop one after overflow: ");
	
	p2 = vnic_input(vnic);
	if(p1 != p2)
		fail("vnic_input returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_input_size(vnic);
	if(size != 0)
		fail("vnic_input_size must be 0: %d", size);
	
	vnic_free(p2);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
	
	pass();


	printf("Slow slow_input queue: Check initial status: ");
	if(vnic_has_slow_input(vnic))
		fail("vnic_has_slow_input must be false");
	
	p1 = vnic_slow_input(vnic);
	if(p1 != NULL)
		fail("vnic_slow_input must be NULL: %p", p1);

	size = vnic_slow_input_size(vnic);
	if(size != 0)
		fail("vnic_slow_input_size must be 0: %d", size);
	
	if(!vnic_slow_receivable(vnic))
		fail("vnic_slow_receivable must be true");
	
	pass();


	printf("Slow slow_input queue: push one: ");
	p1 = vnic_alloc(vnic, 64);
	if(!vnic_slow_received2(vnic, p1))
		fail("vnic_slow_received must be return true");
	
	if(!vnic_has_slow_input(vnic))
		fail("vnic_has_slow_input must be true");
	
	size = vnic_slow_input_size(vnic);
	if(size != 1)
		fail("vnic_slow_input_size must be 1: %d", size);
	
	pass();
	

	printf("Slow slow_input queue: pop one: ");
	
	p2 = vnic_slow_input(vnic);
	if(p2 != p1)
		fail("vnic_slow_input returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_slow_input_size(vnic);
	if(size != 0)
		fail("vnic_slow_input_size must be 0: %d", size);
	
	vnic_free(p2);
	
	pass();
	

	printf("Slow slow_input queue: push full: ");
	for(i = 0; i < vnic->slow_input.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i);
		
		if(!vnic_slow_received2(vnic, ps[i]))
			fail("cannot push slow_input: count: %d, queue size: %d", i, vnic->slow_input.size);
	}
	
	if(vnic_slow_receivable(vnic))
		fail("vnic_slow_receivable must return false");
	
	size = vnic_slow_input_size(vnic);
	if(size != vnic->slow_input.size - 1)
		fail("vnic_slow_input_size must return %d but %d", vnic->slow_input.size - 1, size);
	
	if(!vnic_has_slow_input(vnic))
		fail("vnic_has_slow_input must return true");
	
	pass();
	

	printf("Slow slow_input queue: overflow: ");

	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("packet allocation failed");
	
	if(vnic_slow_received2(vnic, p1))
		fail("push overflow: count: %d, queue size: %d", i, vnic->slow_input.size);
	
	used2 = bitmap_used(vnic);
	if(used != used2)
		fail("packet pool usage is changed: used: %d != %d", used, used2);
	
	if(vnic_slow_receivable(vnic))
		fail("vnic_slow_receivable must return false");
	
	size = vnic_slow_input_size(vnic);
	if(size != vnic->slow_input.size - 1)
		fail("vnic_slow_input_size must return %d but %d", vnic->slow_input.size - 1, size);
	
	if(!vnic_has_slow_input(vnic))
		fail("vnic_has_slow_input must return true");
	
	pass();
	

	printf("Slow slow_input queue: pop: ");
	for(i = 0; i < vnic->slow_input.size - 1; i++) {
		Packet* p1 = vnic_slow_input(vnic);
		if(p1 != ps[i])
			fail("worong pointer returned: %p, expected: %p", p1, (void*)(uintptr_t)i);

		vnic_free(p1);
	}
	
	if(!vnic_slow_receivable(vnic))
		fail("vnic_slow_receivable must return true");
	
	size = vnic_slow_input_size(vnic);
	if(size != 0)
		fail("vnic_slow_input_size must return 0 but %d", size);
	
	if(vnic_has_slow_input(vnic))
		fail("vnic_has_slow_input must return false");
	
	pass();


	printf("Slow slow_input queue: push one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	if(p1 == NULL)
		fail("cannot alloc packet: %p\n", p1);
	
	if(!vnic_slow_received2(vnic, p1))
		fail("vnic_slow_received must be return true");
	
	if(!vnic_has_slow_input(vnic))
		fail("vnic_has_slow_input must be true");
	
	size = vnic_slow_input_size(vnic);
	if(size != 1)
		fail("vnic_slow_input_size must be 1: %d", size);
	
	pass();
	

	printf("Slow slow_input queue: pop one after overflow: ");
	
	p2 = vnic_slow_input(vnic);
	if(p1 != p2)
		fail("vnic_slow_input returned wrong pointer: %p != %p", p1, p2);
	
	size = vnic_slow_input_size(vnic);
	if(size != 0)
		fail("vnic_slow_input_size must be 0: %d", size);
	
	vnic_free(p2);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
	
	pass();



	printf("Output queue: Check initial state: ");

	size = vnic_output_size(vnic);
	if(size != 0)
		fail("vnic_output_size must be 0: %d", size);
	
	if(!vnic_output_available(vnic))
		fail("vnic_output_available must be true");
	
	if(vnic_sendable(vnic))
		fail("vnic_sendable must false");

	pass();
	

	printf("Output queue: output one: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_output(vnic, p1))
		fail("vnic_output must be true");
	
	size = vnic_output_size(vnic);
	if(size != 1)
		fail("vnic_output_size must be 1: %d", size);
	
	if(!vnic_output_available(vnic))
		fail("vnic_output_available must be true");
	
	if(!vnic_sendable(vnic))
		fail("vnic_sendable must true");

	pass();
	

	printf("Output queue: send one: ");
	p1 = vnic_send(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_output_size(vnic);
	if(size != 0)
		fail("vnic_output_size must be 0: %d", size);
	
	if(!vnic_output_available(vnic))
		fail("vnic_output_available must be true");
	
	if(vnic_sendable(vnic))
		fail("vnic_sendable must false");
	
	pass();


	printf("Output queue: output full: ");
	for(i = 0; i < vnic->output.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i + 1);
		
		if(!vnic_output(vnic, ps[i]))
			fail("cannot output packet: index: %d", i);
	}

	size = vnic_output_size(vnic);
	if(size != vnic->output.size - 1)
		fail("vnic_output_size must be %d: %d", vnic->output.size - 1, size);
	
	if(vnic_output_available(vnic))
		fail("vnic_output_available must be false");
	
	if(!vnic_sendable(vnic))
		fail("vnic_sendable must true");
	
	pass();


	printf("Output queue: overflow: ");
	
	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	
	if(vnic_output(vnic, p1))
		fail("vnic_output overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet not freed on overflow %d != %d", used, used2);
		
	if(vnic_output_try(vnic, p1))
		fail("vnic_output_try overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet freed on output_try %d != %d", used, used2);
	
	if(vnic_output_dup(vnic, p1))
		fail("vnic_output_dup overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet allocated on overflow %d != %d", used, used2);

	size = vnic_output_size(vnic);
	if(size != vnic->output.size - 1)
		fail("vnic_output_size must be %d: %d", vnic->output.size - 1, size);
	
	if(vnic_output_available(vnic))
		fail("vnic_output_available must be false");
	
	if(!vnic_sendable(vnic))
		fail("vnic_sendable must true");
	
	pass();
	
	
	printf("Output queue: send all: ");
	for(i = 0; i < vnic->output.size - 1; i++) {
		p1 = vnic_send(vnic);
		if(p1 != ps[i])
			fail("wrong pointer returned: %p != %p", ps[i], p1);
		
		if(!vnic_free(p1))
			fail("cannot free packet: %p", p1);
	}

	size = vnic_output_size(vnic);
	if(size != 0)
		fail("vnic_output_size must be 0: %d", 0, size);
	
	if(!vnic_output_available(vnic))
		fail("vnic_output_available must be true");
	
	if(vnic_sendable(vnic))
		fail("vnic_sendable must false");
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
		
	pass();


	printf("Output queue: output one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_output(vnic, p1))
		fail("vnic_output must be true");
	
	size = vnic_output_size(vnic);
	if(size != 1)
		fail("vnic_output_size must be 1: %d", size);
	
	if(!vnic_output_available(vnic))
		fail("vnic_output_available must be true");
	
	if(!vnic_sendable(vnic))
		fail("vnic_sendable must true");

	pass();
	

	printf("Output queue: send one after overflow: ");
	p1 = vnic_send(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_output_size(vnic);
	if(size != 0)
		fail("vnic_output_size must be 0: %d", size);
	
	if(!vnic_output_available(vnic))
		fail("vnic_output_available must be true");
	
	if(vnic_sendable(vnic))
		fail("vnic_sendable must false");
	
	pass();


	printf("Slow output queue: Check initial state: ");

	size = vnic_slow_output_size(vnic);
	if(size != 0)
		fail("vnic_slow_output_size must be 0: %d", size);
	
	if(!vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be true");
	
	if(vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must false");

	pass();
	

	printf("Slow output queue: slow_output one: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_slow_output(vnic, p1))
		fail("vnic_slow_output must be true");
	
	size = vnic_slow_output_size(vnic);
	if(size != 1)
		fail("vnic_slow_output_size must be 1: %d", size);
	
	if(!vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be true");
	
	if(!vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must true");

	pass();
	

	printf("Slow output queue: send one: ");
	p1 = vnic_slow_send(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_slow_output_size(vnic);
	if(size != 0)
		fail("vnic_slow_output_size must be 0: %d", size);
	
	if(!vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be true");
	
	if(vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must false");
	
	pass();


	printf("Slow output queue: slow_output full: ");
	for(i = 0; i < vnic->slow_output.size - 1; i++) {
		ps[i] = vnic_alloc(vnic, 0);
		if(ps[i] == NULL)
			fail("cannot alloc packet: count: %d", i + 1);
		
		if(!vnic_slow_output(vnic, ps[i]))
			fail("cannot slow_output packet: index: %d", i);
	}

	size = vnic_slow_output_size(vnic);
	if(size != vnic->slow_output.size - 1)
		fail("vnic_slow_output_size must be %d: %d", vnic->slow_output.size - 1, size);
	
	if(vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be false");
	
	if(!vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must true");
	
	pass();


	printf("Slow output queue: overflow: ");
	
	used = bitmap_used(vnic);
	
	p1 = vnic_alloc(vnic, 0);
	
	if(vnic_slow_output(vnic, p1))
		fail("vnic_slow_output overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet not freed on overflow %d != %d", used, used2);
		
	if(vnic_slow_output_try(vnic, p1))
		fail("vnic_slow_output_try overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet freed on slow_output_try %d != %d", used, used2);
	
	if(vnic_slow_output_dup(vnic, p1))
		fail("vnic_slow_output_dup overflow");
	
	used2 = bitmap_used(vnic);
	
	if(used != used2)
		fail("packet allocated on overflow %d != %d", used, used2);

	size = vnic_slow_output_size(vnic);
	if(size != vnic->slow_output.size - 1)
		fail("vnic_slow_output_size must be %d: %d", vnic->slow_output.size - 1, size);
	
	if(vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be false");
	
	if(!vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must true");
	
	pass();
	
	
	printf("Slow output queue: send all: ");
	for(i = 0; i < vnic->slow_output.size - 1; i++) {
		p1 = vnic_slow_send(vnic);
		if(p1 != ps[i])
			fail("wrong pointer returned: %p != %p", ps[i], p1);
		
		if(!vnic_free(p1))
			fail("cannot free packet: %p", p1);
	}

	size = vnic_slow_output_size(vnic);
	if(size != 0)
		fail("vnic_slow_output_size must be 0: %d", 0, size);
	
	if(!vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be true");
	
	if(vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must false");
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("packet is not freed: used: %d", used);
		
	pass();


	printf("Slow output queue: slow_output one after overflow: ");
	p1 = vnic_alloc(vnic, 0);
	
	if(!vnic_slow_output(vnic, p1))
		fail("vnic_slow_output must be true");
	
	size = vnic_slow_output_size(vnic);
	if(size != 1)
		fail("vnic_slow_output_size must be 1: %d", size);
	
	if(!vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be true");
	
	if(!vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must true");

	pass();
	

	printf("Slow output queue: send one after overflow: ");
	p1 = vnic_slow_send(vnic);
	vnic_free(p1);
	
	used = bitmap_used(vnic);
	if(used != 0)
		fail("cannot free packet: %d", used);
	
	size = vnic_slow_output_size(vnic);
	if(size != 0)
		fail("vnic_slow_output_size must be 0: %d", size);
	
	if(!vnic_slow_output_available(vnic))
		fail("vnic_slow_output_available must be true");
	
	if(vnic_slow_sendable(vnic))
		fail("vnic_slow_sendable must false");
	
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
