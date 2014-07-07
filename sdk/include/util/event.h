#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef bool(*EventFunc)(void* context);
typedef bool(*TriggerEventFunc)(int event_id, void* event, void* context);

void event_init();
int event_loop();

uint64_t event_busy_add(EventFunc func, void* context);
bool event_busy_remove(uint64_t id);

uint64_t event_timer_add(EventFunc func, void* context, clock_t delay, clock_t period);
bool event_timer_remove(uint64_t id);

uint64_t event_trigger_add(int event_id, TriggerEventFunc func, void* context);
bool event_trigger_remove(uint64_t id);
void event_trigger_fire(int event_id, void* event);

uint64_t event_idle_add(EventFunc func, void* context);
bool event_idle_remove(uint64_t id);

#endif /* __EVENT_H__ */
