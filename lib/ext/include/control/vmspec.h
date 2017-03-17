#ifndef __CONTROL_VMSPEC_H__
#define __CONTROL_VMSPEC_H__

#include <stdint.h>

typedef enum {
	VM_STATUS_STOP		= 0,
	VM_STATUS_PAUSE		= 1,
	VM_STATUS_START		= 2,
	VM_STATUS_RESUME	= 3,
	VM_STATUS_INVALID	= -1,
} VMStatus;

#define VM_MAX_NIC_COUNT	64
#define VM_MAX_ARGC		256
#define VM_MAX_ARGV		4096

#define NICSPEC_DEVICE_MAC	((uint64_t)1 << 48)

typedef struct {
	uint64_t	mac;
	//uint32_t	port;
	char*		dev;
	uint32_t	input_buffer_size;
	uint32_t	output_buffer_size;
	uint8_t		padding_head;
	uint8_t		padding_tail;
	uint64_t	input_bandwidth;
	uint64_t	output_bandwidth;
	uint32_t	pool_size;
} NICSpec;

typedef struct {
	uint32_t	id;
	
	uint32_t	core_size;
	uint32_t	memory_size;
	uint32_t	storage_size;
	
	uint16_t	nic_count;
	NICSpec*	nics;
	
	uint16_t	argc;
	char**		argv;
} VMSpec;

#endif /* __CONTROL_VMSPEC_H__ */
