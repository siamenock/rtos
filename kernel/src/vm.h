#ifndef __VM_H__
#define __VM_H__

#include "mp.h"
#include "ni.h"

#define EVENT_VM_STARTED	0x0200000000000001
#define EVENT_VM_STOPPED	0x0200000000000002

#define VM_MEMORY_SIZE_ALIGN		0x200000
#define VM_STORAGE_SIZE_ALIGN		0x200000

#define VM_STATUS_STOPPED	0
#define VM_STATUS_PAUSED	1
#define VM_STATUS_STARTED	2

typedef struct {
	uint32_t	count;
	void**		blocks;	// gmalloc(array), bmalloc(content)
} Block;

typedef struct {
	uint64_t	id;
	uint32_t	core_size;
	uint8_t		cores[MP_MAX_CORE_COUNT];
	Block		memory;
	Block		storage;
	uint16_t	nic_size;
	NI**		nics;	// gmalloc, ni_create
	
	int		argc;
	char**		argv;	// gmalloc
	
	uint8_t		status;
} VM;

typedef struct {
	uint8_t		core_size;
	uint8_t		cores[MP_MAX_CORE_COUNT];
	uint32_t	heap_size;
	uint32_t	stack_size;
	uint32_t	global_heap_size;
	uint32_t	storage_size;
	
	NI**		nis;
} VM2;

#endif /* __VM_H__ */
