#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <stdint.h>
#include <stdbool.h>

enum {
	EVENT_VM_STARTED = 101,
	EVENT_VM_STOPPED,
} Events;

#endif /* __EVENTS_H__ */
