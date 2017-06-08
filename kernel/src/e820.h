#ifndef __E820_H__
#define __E820_H__

#include <stdint.h>
#include <util/list.h>

typedef struct {
	uintptr_t start;
	uintptr_t end;
} Block;

List* e820_get_mem_blocks();

#endif /*__E820_H__*/
