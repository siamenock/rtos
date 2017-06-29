#ifndef __SHARED_H__
#define __SHARED_H__

#define SHARED_MAGIC		0x481230420134f090

#ifndef __ASSEMBLY__

#include <stdint.h>
#include "mp.h"
struct _FIFO;

typedef struct {
	struct _FIFO*		icc_queue;
	volatile uint8_t 	icc_queue_lock;
} ICC;

/**
 * Shared Memeory Structure
 *
 * *IMPORTANT* If you change this structure, the relevant code
 * in the entry.S file must also be changed.
 */
typedef struct {
	volatile uint8_t	mp_processors[MP_MAX_CORE_COUNT];

	volatile uint8_t    	sync;

	struct _FIFO*		icc_pool;
	volatile uint8_t	icc_lock_alloc;
	volatile uint8_t	icc_lock_free;
	ICC*			icc_queues;

	uint64_t		magic;
} __attribute__ ((packed)) Shared;

int shared_init();
void shared_sync();

#endif /* __ASSEMBLY__ */

#endif /* __SHARED_H__ */
