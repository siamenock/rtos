#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdint.h>
#include <stdbool.h>

enum {
	EVENT_VM_STARTED = 1,
	EVENT_VM_STOPPED,
} Events;

void event_init();
void event_loop();

uint64_t event_event(int event_type, bool(*func)(int,void*,void*), void* context);
bool event_event_cancel(uint64_t id);
void event_trigger(int event_type, void* event);

void event_once(void(*func)(void*), void* context);
uint64_t event_tevent(bool(*func)(void*), void* context, uint64_t delay, uint64_t period);
bool event_tevent_cancel(uint64_t id);
void event_idle(void(*func)(void*), void* context);
void event_busy(void(*func)(void*), void* context);

#endif /* __EVENT_H__ */
