#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <malloc.h>
#include "command.h"
#include "vnic.h"

#define MAX_VNIC_COUNT	4
#define MSB		((uint64_t)1 << 63)

int ports[MAX_VNIC_COUNT];
VNIC* vnics[MAX_VNIC_COUNT];

int vnic_create(int idx, void* addr, size_t size) {
	printf("vnic create: %d %p %ld\n", idx, addr, size);
	int id = 0;
	VNIC* vnic = NULL;
	for(id = 0; id < MAX_VNIC_COUNT; id++) {
		if(vnics[id] == NULL) {
			vnic = addr;
			break;
		}
	}
	
	if(!vnic)
		return false;
	
	bzero(addr, size);
	vnic = addr;
	
	vnic->prealloc_count = 8;
	vnic->fifo_count = 128;
	vnic->id_type = VNIC_ID_TYPE_MAC;
	vnic->id = 0x001122334455;
	vnic->input_bandwidth = 10000000;	// 10Mbps
	vnic->output_bandwidth = 10000000;	// 10Mbps
	vnic->packet_header = 32;
	vnic->packet_align = 8;
	vnic->packet_size = 4;
	vnic->packet_start = 0;
	vnic->input_accept_size = 1;
	vnic->output_accept_size = 1;
	vnic->metadata_size = sizeof(VNIC) + sizeof(uint64_t) * (vnic->input_accept_size + vnic->output_accept_size);
	VNIC_INPUT_ACCEPT_LIST(vnic)[0] = 0;
	VNIC_OUTPUT_ACCEPT_LIST(vnic)[0] = 0;
	
	printf("input_accept_list: %ld\n", (void*)VNIC_INPUT_ACCEPT_LIST(vnic) - addr);
	printf("output_accept_list: %ld\n", (void*)VNIC_OUTPUT_ACCEPT_LIST(vnic) - addr);
	printf("malloc: %ld\n", (void*)VNIC_MALLOC(vnic) - addr);
	printf("malloc_64: %ld\n", (void*)VNIC_MALLOC_64(vnic) - addr);
	printf("malloc_128: %ld\n", (void*)VNIC_MALLOC_128(vnic) - addr);
	printf("malloc_256: %ld\n", (void*)VNIC_MALLOC_256(vnic) - addr);
	printf("malloc_512: %ld\n", (void*)VNIC_MALLOC_512(vnic) - addr);
	printf("malloc_1024: %ld\n", (void*)VNIC_MALLOC_1024(vnic) - addr);
	printf("malloc_1514: %ld\n", (void*)VNIC_MALLOC_1514(vnic) - addr);
	printf("malloc_END: %ld\n", (void*)VNIC_MALLOC_END(vnic) - addr);
	printf("fifo_input: %ld\n", (void*)VNIC_INPUT(vnic) - addr);
	printf("fifo_input_head: %ld\n", (void*)VNIC_INPUT_HEAD(vnic) - addr);
	printf("fifo_input_tail: %ld\n", (void*)VNIC_INPUT_TAIL(vnic) - addr);
	printf("fifo_input_fifo: %ld\n", (void*)VNIC_INPUT_FIFO(vnic) - addr);
	printf("fifo_output: %ld\n", (void*)VNIC_OUTPUT(vnic) - addr);
	printf("fifo_output_head: %ld\n", (void*)VNIC_OUTPUT_HEAD(vnic) - addr);
	printf("fifo_output_tail: %ld\n", (void*)VNIC_OUTPUT_TAIL(vnic) - addr);
	printf("fifo_output_fifo: %ld\n", (void*)VNIC_OUTPUT_FIFO(vnic) - addr);
	
#define __MALLOC_INIT(SIZE) \
	for(int i = 0; i < vnic->prealloc_count; i++) {					\
		void* ptr = malloc(vnic->packet_header + (SIZE) + 32 /* padding */);	\
		*(uint32_t*)ptr = (SIZE);						\
		printf("Malloc_%d: %d %p\n", SIZE, i, ptr);				\
		VNIC_MALLOC_##SIZE(vnic)[i] = (uint64_t)(ptr + 4);			\
	}

	__MALLOC_INIT(64);
	__MALLOC_INIT(128);
	__MALLOC_INIT(256);
	__MALLOC_INIT(512);
	__MALLOC_INIT(1024);
	__MALLOC_INIT(1514);
	
	if(vnic_add(idx, (uint64_t)addr)) {
		vnics[id] = vnic;
		ports[id] = idx;
		return id;
	} else {
		return -1;
	}
}

bool vnic_destroy(int id) {
	if(vnics[id]) {
		vnics[id] = NULL;
		vnic_remove(ports[id], (uint64_t)vnics[id]);
		return true;
	} else {
		return false;
	}
}

typedef struct {
	uint32_t	start;
	uint32_t	end;
	uint32_t	size;
	uint32_t	padding;
	uint8_t		data[0];
} __attribute__((packed)) Packet;

bool vnic_loop(int id) {
	VNIC* vnic = vnics[id];
	
	// Malloc
#define __MALLOC_PAD(SIZE) \
	for(int i = 0; i < vnic->prealloc_count; i++) {						\
		if(VNIC_MALLOC_##SIZE(vnic)[i] & MSB) {						\
			void* ptr = malloc(vnic->packet_header + (SIZE) + 32 /* padding */);	\
			printf("Malloc_%d: %d %p\n", SIZE, i, ptr);				\
			*(uint32_t*)ptr = (SIZE);						\
			VNIC_MALLOC_##SIZE(vnic)[i] = (uint64_t)(ptr + 4);			\
		}										\
	}
	__MALLOC_PAD(64);
	__MALLOC_PAD(128);
	__MALLOC_PAD(256);
	__MALLOC_PAD(512);
	__MALLOC_PAD(1024);
	__MALLOC_PAD(1514);
	
	// Input
	uint32_t* head = VNIC_INPUT_HEAD(vnic);
	uint32_t* tail = VNIC_INPUT_TAIL(vnic);
	while(*head != *tail) {
		uint32_t idx = *head;
		void* ptr = (void*)VNIC_INPUT_FIFO(vnic)[idx];
		uint32_t size = *(uint32_t*)(ptr - 4);
		Packet* packet = ptr;
		packet->end += vnic->packet_header;
		packet->size = size;
		 
		printf("Input[%d]: %d/%d/%d ", ports[id], packet->start, packet->end, packet->size);
		int pos = packet->start;
		for(int i = 0; i < 6; i++) {
			printf("%02x", packet->data[pos++]);
		}
		printf(" ");
		for(int i = 0; i < 6; i++) {
			printf("%02x", packet->data[pos++]);
		}
		printf(" ");
		for(int i = 0; i < 2; i++) {
			printf("%02x", packet->data[pos++]);
		}
		printf(" ");
		for(int i = 0; i < 8; i++) {
			printf("%02x", packet->data[pos++]);
		}
		printf("\n");
		
		if(++idx >= vnic->fifo_count)
			idx = 0;
		*head = idx;
		
		// Output
		uint32_t* head2 = VNIC_OUTPUT_HEAD(vnic);
		uint32_t* tail2 = VNIC_OUTPUT_TAIL(vnic);
		uint32_t idx2 = *tail2;
		if((idx2 + 1) % vnic->fifo_count == *head2) {
			printf("Error cannout output packet: %d %d\n", *head2, *tail2);
			break;
		}
		
		// Free
		if(VNIC_OUTPUT_FIFO(vnic)[idx2] & MSB) {
			void* ptr = (void*)VNIC_OUTPUT_FIFO(vnic)[idx2];
			uint32_t size = *(uint32_t*)(ptr - 4);
			printf("Free_%d: %p\n", size, ptr - 4);
			free(ptr - 4);
		}
		
		VNIC_OUTPUT_FIFO(vnic)[idx2] = (uint64_t)packet;
		if(++idx2 >= vnic->fifo_count)
			idx2 = 0;
		*tail2 = idx2;
		
		vnic_output(ports[id], (uint64_t)vnics[id]);
	}
	
	return true;
}

int main(int argc, char** argv) {
	int ids[4];
	
	for(int i = 0; i < 4; i++)
		ids[i] = vnic_create(i, malloc(4096), 4096);
	
	while(1) {
		for(int i = 0; i < 4; i++) {
			vnic_loop(ids[i]);
			usleep(10);
		}
	}
	
	return 0;
}
