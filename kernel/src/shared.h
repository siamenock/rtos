#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdint.h>

struct _PCI_Device;

typedef struct {
	volatile uint8_t	mp_sync_lock;
	volatile uint32_t	mp_sync_map;
	volatile uint32_t	mp_wait_map;
	
	struct _ICC_Message*	icc_messages;
} Shared;

Shared* shared;

void shared_init();

#endif /* __SHARED_H__ */
