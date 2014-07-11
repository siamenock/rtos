#include <string.h>
#include <lock.h>
#include <net/ni.h>
#include <tlsf.h>

int __nis_count;
NetworkInterface* __nis[NIS_SIZE];

#define NI_POOL(ptr)	(void*)((uint64_t)(void*)(ptr) & ~(0x200000 - 1))
#define ALIGN           16

inline int ni_count() {
	return __nis_count;
}

NetworkInterface* ni_get(int i) {
	if(i < __nis_count)
		return __nis[i];
	else
		return NULL;
}

Packet* ni_alloc(NetworkInterface* ni, uint16_t size) {
	void* pool = NI_POOL(ni);
	Packet* packet = malloc_ex(sizeof(Packet) + size + ALIGN - 1, pool);
	if(packet) {
		bzero(packet, sizeof(Packet));
		packet->ni = ni;
		packet->size = size + ALIGN - 1;
		packet->end = packet->start = (((uint64_t)packet->buffer + ALIGN - 1) & ~(ALIGN - 1)) - (uint64_t)packet->buffer;
	}
	
	return packet;
}

inline void ni_free(Packet* packet) {
	free_ex(packet, NI_POOL(packet->ni));
}

inline bool ni_has_input(NetworkInterface* ni) {
	return !fifo_empty(ni->input_buffer);
}

inline Packet* ni_tryinput(NetworkInterface* ni) {
	if(lock_trylock(&ni->input_lock)) {
		Packet* packet = fifo_pop(ni->input_buffer);
		lock_unlock(&ni->input_lock);
		
		return packet;
	} else {
		return NULL;
	}
}

inline Packet* ni_input(NetworkInterface* ni) {
	lock_lock(&ni->input_lock);
	
	Packet* packet = fifo_pop(ni->input_buffer);
	
	lock_unlock(&ni->input_lock);
	
	return packet;
}

inline bool ni_output_available(NetworkInterface* ni) {
	return fifo_available(ni->output_buffer);
}

inline bool ni_has_output(NetworkInterface* ni) {
	return !fifo_empty(ni->output_buffer);
}

bool ni_output(NetworkInterface* ni, Packet* packet) {
	lock_lock(&ni->output_lock);
	
	if(fifo_push(ni->output_buffer, packet)) {
		lock_unlock(&ni->output_lock);
		return true;
	} else {
		ni->output_drop_bytes += packet->end - packet->start;
		ni->output_drop_packets++;
		
		lock_unlock(&ni->output_lock);
		
		ni_free(packet);
		
		return false;
	}
}

bool ni_output_dup(NetworkInterface* ni, Packet* packet) {
	uint16_t len = packet->end - packet->start;
	
	Packet* p = ni_alloc(ni, len);
	if(!p)
		return false;
	
	// Copy packet data
	uint16_t start = p->start;
	uint16_t size = p->size;
	
	memcpy(p, packet, sizeof(Packet));
	p->ni = ni;
	p->start = start;
	p->size = size;
	p->end = p->start + len;
	
	// Copy packet buffer
	memcpy(p->buffer + p->start, packet->buffer + packet->start, len);
	
	if(!ni_output(ni, p)) {
		ni_free(p);
		return false;
	} else {
		return true;
	}
}

inline bool ni_tryoutput(NetworkInterface* ni, Packet* packet) {
	if(lock_trylock(&ni->output_lock)) {
		bool result = fifo_push(ni->output_buffer, packet);
		lock_unlock(&ni->output_lock);
		
		return result;
	} else {
		return false;
	}
}

size_t ni_pool_used(NetworkInterface* ni) {
	void* pool = NI_POOL(ni);
	return get_used_size(pool);
}

size_t ni_pool_free(NetworkInterface* ni) {
	void* pool = NI_POOL(ni);
	return get_total_size(pool) - get_used_size(pool);
}

size_t ni_pool_total(NetworkInterface* ni) {
	void* pool = NI_POOL(ni);
	return get_total_size(pool);
}

