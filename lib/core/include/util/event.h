#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * @file
 * PacketNgin event engine
 * Busy event - called whenever event_loop() is called
 * Trigger event - called when a event is triggered
 * Timer event - called regularly
 * Idle event - called there is no timer or trigger event to be called
 * Event calling priority: busy > trigger > timer > idle
 */

/**
 * Busy or timer event callback
 * context pointer will be passed whenever the callback is called.
 */
typedef bool(*EventFunc)(void* context);

/**
 * Triggered event callback
 * context pointer will be passed whenever the callback is called.
 */
typedef bool(*TriggerEventFunc)(uint64_t event_id, void* event, void* context);

/**
 * Initialize event engine
 */
void event_init();

/**
 * Process events
 *
 * @return number of events processed
 */
int event_loop();

/**
 * Register busy event which is called every time when event loop is called
 *
 * @param func event callback
 * @param context the callback's context
 * @return ID of the callback
 */
uint64_t event_busy_add(EventFunc func, void* context);

/**
 * Deregister busy event
 *
 * @param id busy event ID
 * @return true if deregistered
 */
bool event_busy_remove(uint64_t id);

/**
 * Register trigger event
 *
 * @param event_id the event
 * @param func event callback
 * @param context the callback's context
 * @return event ID
 */
uint64_t event_trigger_add(uint64_t event_id, TriggerEventFunc func, void* context);

/**
 * Deregister tirgger event
 *
 * @param id event ID
 * @return true if deregistered
 */
bool event_trigger_remove(uint64_t id);

/**
 * Fire a event, related trigger events will be called next event loop.
 *
 * @param event_id the event
 * @param event event data
 * @param last the last callback when every trigger event is called
 * @param last_context the last callback's context
 */
void event_trigger_fire(uint64_t event_id, void* event, TriggerEventFunc last, void* last_context);

/**
 * Stop to propagate trigger events
 */
void event_trigger_stop();

/**
 * Register timer event
 *
 * @param func event callback
 * @param context callback's context
 * @param delay callback will be called after delay
 * @param period callback will be called regularly in every period
 * @return event ID
 */
uint64_t event_timer_add(EventFunc func, void* context, clock_t delay, clock_t period);

/**
 * Update timer event
 *
 * @param id event ID
 * @param period callback will be called regularly in every period
 * @return true if deregistered
 */
bool event_timer_update(uint64_t id, clock_t period);

/**
 * Deregister timer event
 *
 * @param id event ID
 * @return true if deregistered
 */
bool event_timer_remove(uint64_t id);

/**
 * Register idle event
 *
 * @param func event callback
 * @param context the callback's context
 * @return event ID
 */
uint64_t event_idle_add(EventFunc func, void* context);

/**
 * Deregister idle event
 *
 * @param id event ID
 * @return true if deregistered
 */
bool event_idle_remove(uint64_t id);

#endif /* __EVENT_H__ */
