#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdint.h>
#include "mp.h"

#define SHARED_MAGIC		0x481230420134f090

struct _FIFO;

typedef struct {
	struct _FIFO*		icc_queue;
	volatile uint8_t 	icc_queue_lock;
} Icc;

typedef struct {
	// Core
	uint8_t			mp_cores[MP_MAX_CORE_COUNT];

	// Memory
	uint64_t*		gmalloc_pool;
	uint32_t		bmalloc_count;
	uint64_t*		bmalloc_pool;

	struct _FIFO*		icc_pool;
	volatile uint8_t	icc_lock_alloc;
	volatile uint8_t	icc_lock_free;

	Icc*			icc_queues;

	// Magic
	uint64_t		magic;
} Shared;

Shared* shared;

int shared_init();

#endif /* __SHARED_H__ */
