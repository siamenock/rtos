#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>
#include <stdbool.h>

enum {
	RESOURCE_NI,
};

enum {
	SYM_NIS_COUNT,
	SYM_NIS,
	SYM_DPIS,
	SYM_APP_STATUS,
	SYM_MALLOC_POOL,
	SYM_STDIN,
	SYM_STDIN_HEAD,
	SYM_STDIN_TAIL,
	SYM_STDIN_SIZE,
	SYM_STDOUT,
	SYM_STDOUT_HEAD,
	SYM_STDOUT_TAIL,
	SYM_STDOUT_SIZE,
	SYM_STDERR,
	SYM_STDERR_HEAD,
	SYM_STDERR_TAIL,
	SYM_STDERR_SIZE,
	SYM_GMALLOC_POOL,
	SYM_THREAD_ID,
	SYM_THREAD_COUNT,
	SYM_BARRIOR_LOCK,
	SYM_BARRIOR,
	SYM_SHARED,
	SYM_CPU_FREQUENCY,
	SYM_FIO,
	SYM_END
};

extern char* task_symbols[];

void task_init();
uint32_t task_create();
void task_entry(uint32_t id, uint64_t entry);
void task_stack(uint32_t id, uint64_t stack);
void task_arguments(uint32_t id, uint64_t argc, uint64_t argv);
void task_mmap(uint32_t id, uint64_t vaddr, uint64_t paddr, bool is_user, bool is_writable, bool is_executable, char* desc);
void task_refresh_mmap();
void task_resource(uint32_t id, uint8_t type, void* data);
void task_symbol(uint32_t id, uint32_t symbol, uint64_t vaddr);
void* task_addr(uint32_t id, uint32_t symbol);
void task_destroy(uint32_t id);

uint32_t task_id();
void task_switch(uint32_t id);
bool task_is_active(uint32_t id);
uint64_t task_get_stack(uint32_t id);

void task_dump(uint32_t id);

#endif /* __TASK_H__ */
