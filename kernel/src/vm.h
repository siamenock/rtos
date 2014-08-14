#ifndef __VM_H__
#define __VM_H__

#include "mp.h"
#include "ni.h"

#define EVENT_VM_STARTED	0x0200000000000001
#define EVENT_VM_PAUSED		0x0200000000000002
#define EVENT_VM_RESUMED	0x0200000000000003
#define EVENT_VM_STOPPED	0x0200000000000004

#define VM_MEMORY_SIZE_ALIGN	0x200000
#define VM_STORAGE_SIZE_ALIGN	0x200000

#define VM_STATUS_STOP		0
#define VM_STATUS_PAUSE		1
#define VM_STATUS_START		2
#define VM_STATUS_RESUME	3

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
	uint64_t	mac;
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

void vm_init();

uint64_t vm_create(VMSpec* vm_spec);
bool vm_delete(uint64_t vmid);
int vm_count();
bool vm_contains(uint64_t vmid);
int vm_list(uint64_t* vmids, int size);

typedef void(*VM_STATUS_CALLBACK)(bool, void*);
void vm_status_set(uint64_t vmid, int status, VM_STATUS_CALLBACK callback, void* context);
int vm_status_get(uint64_t vmid);

size_t vm_storage_read(uint64_t vmid, void* buf, size_t offset, size_t size);
size_t vm_storage_write(uint64_t vmid, void* buf, size_t offset, size_t size);
bool vm_storage_md5(uint64_t vmid, uint32_t size, uint32_t digest[4]);

typedef void(*VM_STDIO_CALLBACK)(uint64_t vmid, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size);
void vm_stdio(VM_STDIO_CALLBACK callback);

#endif /* __VM_H__ */
