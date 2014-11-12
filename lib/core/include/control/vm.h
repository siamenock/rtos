#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>
#include <stdbool.h>

#define VM_STATUS_STOP		0
#define VM_STATUS_PAUSE		1
#define VM_STATUS_RESUME	2
#define VM_STATUS_START		3
#define VM_STATUS_INVALID	-1

#define PN_MAX_NIC_COUNT	32
#define PN_MAX_ARGS_SIZE	4096

typedef struct {
	uint64_t	mac;
	uint32_t	port;
	uint32_t	input_buffer_size;
	uint32_t	output_buffer_size;
	uint64_t	input_bandwidth;
	uint64_t	output_bandwidth;
	uint32_t	pool_size;
} NICSpec;

typedef struct {
	uint32_t	core_size;
	uint32_t	memory_size;
	uint32_t	storage_size;
	
	uint32_t	nic_size;
	NICSpec*	nics;
	
	int		argc;
	char**		argv;
} VMSpec;

#endif /* __VM_H__ */
