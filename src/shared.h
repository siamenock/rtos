#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdint.h>
#include "mp.h"

struct _FIFO;

typedef struct {
	struct _FIFO*		icc_queue;
	volatile uint8_t 	icc_queue_lock;
} Icc;

/*
 *        char* __stdin = shared->__stdin;
 *        size_t __stdin_head;
 *        size_t __stdin_tail;
 *        size_t __stdin_size;
 *
 *        char __stdout[BUFFER_SIZE];
 *        size_t __stdout_head;
 *        size_t __stdout_tail;
 *        size_t __stdout_size = BUFFER_SIZE;
 *
 *        char __stderr[BUFFER_SIZE];
 *        size_t __stderr_head;
 *        size_t __stderr_tail;
 *        size_t __stderr_size = BUFFER_SIZE;
 *
 */
typedef struct {
    // Standard I/O
    char**              __stdout;
    size_t*             __stdout_head;
    size_t*             __stdout_tail;
    size_t*             __stdout_size;

    // Core
    uint8_t             mp_cores[MP_MAX_CORE_COUNT];

    // Memory
    uint32_t            bmalloc_count;
    uint64_t*           bmalloc_pool;

	struct _FIFO*		icc_pool;
	volatile uint8_t	icc_lock_alloc;
	volatile uint8_t	icc_lock_free;

	Icc*			    icc_queues;
} Shared;

Shared* shared;

void shared_init();

#endif /* __SHARED_H__ */
