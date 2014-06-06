#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "page.h"

typedef struct {
	PageDirectory pml4[512];
	PageDirectory pdp[512];
	PageTable pd[512];
} __attribute__((packed)) Process;

void process_init();
uint32_t process_create(void(*entry)(void*), void* data);
void process_destroy(uint32_t id);

uint32_t process_id();
void process_switch(uint32_t id);
void process_wait(uint64_t ms);

void process_dump(uint32_t id);

#endif /* __PROCESS_H__ */
