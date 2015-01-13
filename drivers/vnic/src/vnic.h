#ifndef __VNIC_H__
#define __VNIC_H__

#include <stdint.h>
#include <stdbool.h>

#define VNIC_ID_TYPE_MAC	0
#define VNIC_ID_TYPE_VLAN	1

typedef struct _VNIC {
	/* metadata */
	uint32_t metadata_size;
	uint32_t prealloc_count;
	uint32_t fifo_count;
	uint32_t id_type;	// 0 is mac 1 is VLAN id
	uint64_t id;
	uint64_t input_bandwidth;
	uint64_t output_bandwidth;	// 40
	uint32_t input_drop_count;
	uint32_t output_drop_count;
	uint32_t packet_header;
	uint32_t packet_align;
	uint32_t packet_size;		// 60
	uint32_t packet_start;
	uint32_t input_accept_size;
	uint32_t output_accept_size;	// 72
	#define VNIC_INPUT_ACCEPT_LIST(vnic)	((uint64_t*)((void*)&(vnic)->output_accept_size + sizeof(uint32_t)))
	#define VNIC_OUTPUT_ACCEPT_LIST(vnic)	((uint64_t*)((void*)&(vnic)->output_accept_size + sizeof(uint32_t) + sizeof(uint64_t) * (vnic)->input_accept_size))
	//uint64_t[] input_accept_list;
	//uint64_t[] output_accept_list;
	
	/* malloc */
	#define VNIC_MALLOC(vnic)		((uint64_t*)((void*)(vnic) + (vnic)->metadata_size))
	#define VNIC_MALLOC_64(vnic)		VNIC_MALLOC(vnic)
	#define VNIC_MALLOC_128(vnic)		(VNIC_MALLOC(vnic) + (vnic)->prealloc_count * 1)
	#define VNIC_MALLOC_256(vnic)		(VNIC_MALLOC(vnic) + (vnic)->prealloc_count * 2)
	#define VNIC_MALLOC_512(vnic)		(VNIC_MALLOC(vnic) + (vnic)->prealloc_count * 3)
	#define VNIC_MALLOC_1024(vnic)		(VNIC_MALLOC(vnic) + (vnic)->prealloc_count * 4)
	#define VNIC_MALLOC_1514(vnic)		(VNIC_MALLOC(vnic) + (vnic)->prealloc_count * 5)
	#define VNIC_MALLOC_END(vnic)		(VNIC_MALLOC(vnic) + (vnic)->prealloc_count * 6)
	//uint64_t[] malloc_64;
	//uint64_t[] malloc_128;
	//uint64_t[] malloc_256;
	//uint64_t[] malloc_512;
	//uint64_t[] malloc_1024;
	//uint64_t[] malloc_1514;
	
	/* fifo */
	#define VNIC_INPUT(vnic)	((void*)VNIC_MALLOC_END(vnic))
	#define VNIC_INPUT_HEAD(vnic)	((uint32_t*)VNIC_INPUT(vnic))
	#define VNIC_INPUT_TAIL(vnic)	((uint32_t*)(VNIC_INPUT(vnic) + sizeof(uint32_t)))
	#define VNIC_INPUT_FIFO(vnic)	((uint64_t*)(VNIC_INPUT(vnic) + sizeof(uint32_t) * 2))
	
	#define VNIC_OUTPUT(vnic)	(VNIC_INPUT(vnic) + sizeof(uint32_t) * 2 + sizeof(uint64_t) * (vnic)->fifo_count)
	#define VNIC_OUTPUT_HEAD(vnic)	((uint32_t*)VNIC_OUTPUT(vnic))
	#define VNIC_OUTPUT_TAIL(vnic)	((uint32_t*)(VNIC_OUTPUT(vnic) + sizeof(uint32_t)))
	#define VNIC_OUTPUT_FIFO(vnic)	((uint64_t*)(VNIC_OUTPUT(vnic) + sizeof(uint32_t) * 2))
	//uint32_t input_head;
	//uint32_t input_tail;
	//uint64_t[] input_fifo;
	//uint32_t output_head;
	//uint32_t output_tail;
	//uint64_t[] output_fifo;
} __attribute__((packed)) VNIC;

int vnic_create(int idx, void* addr, size_t size);
bool vnic_destroy(int id);
bool vnic_loop(int id);

#endif /* __VNIC_H__ */
