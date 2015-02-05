#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdint.h>

struct _PCI_Device;
struct _FIFO;

typedef struct {
	struct _FIFO*		icc_queue;
	volatile uint8_t 	icc_queue_lock;
} Icc;

typedef struct {
	volatile uint8_t	mp_sync_lock;
	volatile uint32_t	mp_sync_map;
	volatile uint32_t	mp_wait_map;
	
	struct _FIFO*		icc_pool;
	volatile uint8_t	icc_lock_alloc;
	volatile uint8_t	icc_lock_free;

	Icc*			icc_queues;
} Shared;

Shared* shared;

void shared_init();

#endif /* __SHARED_H__ */
