#include <malloc.h>
#include <util/list.h>
#include "cpu.h"
#include "event.h"

typedef struct {
	int		event_type;
	bool		(*func)(int,void*,void*);
	void*		context;
} TriggeredEvent;

typedef struct {
	int		event_type;
	void*		event;
} Trigger;

typedef struct {
	void		(*func)(void*);
	void*		context;
} Event;

typedef struct {
	bool		(*func)(void*);
	void*		context;
	
	uint64_t	time;
	uint64_t	period;
} TimedEvent;

static List* events;
static List* triggers;
static List* oevents;
static List* tevents;
static List* bevents;
static List* idles;

static bool get_first_bigger(void* time, void* event) {
	return (uint64_t)time > ((TimedEvent*)event)->time;
}

void event_init() {
	events = list_create(malloc, free);
	triggers = list_create(malloc, free);
	
	oevents = list_create(malloc, free);
	
	tevents = list_create(malloc, free);
	
	idles = list_create(malloc, free);
	
	bevents = list_create(malloc, free);
}

static bool is_tevent_processed = false;

void event_loop() {
	ListIterator iter;
	list_iterator_init(&iter, bevents);
	while(list_iterator_has_next(&iter)) {
		Event* event = list_iterator_next(&iter);
		event->func(event->context);
	}
	
	while(!list_is_empty(triggers)) {
		Trigger* trigger = list_remove_first(triggers);
		
		ListIterator iter;
		list_iterator_init(&iter, events);
		while(list_iterator_has_next(&iter)) {
			TriggeredEvent* event = list_iterator_next(&iter);
			if(event->event_type == trigger->event_type) {
				if(!event->func(trigger->event_type, event->context, trigger->event)) {
					list_iterator_remove(&iter);
					free(event);
				}	
			}
		}
		
		free(trigger);
	}
	
	uint64_t time = cpu_tsc();
	if(!is_tevent_processed && !list_is_empty(tevents) && ((TimedEvent*)list_get_first(tevents))->time <= time) {
		TimedEvent* event = list_remove_first(tevents);
		if(event->func(event->context)) {
			event->time += event->period;
			
			int index = list_index_of(tevents, (void*)event->time, get_first_bigger);
			list_add_at(tevents, index, event);
		} else {
			free(event);
		}
		
		is_tevent_processed = true;
		
		return;
	} 
	
	is_tevent_processed = false;
	
	if(!list_is_empty(oevents)) {
		Event* event = list_remove_first(oevents);
		event->func(event->context);
		free(event);
		
		return;
	}
	
	if(!list_is_empty(idles)) {
		Event* event = list_get_first(idles);
		event->func(event->context);
		list_rotate(idles);
	}
}

uint64_t event_event(int event_type, bool(*func)(int,void*,void*), void* context) {
	TriggeredEvent* event = malloc(sizeof(TriggeredEvent));
	event->event_type = event_type;
	event->func = func;
	event->context = context;
	
	list_add(events, event);
	
	return (uint64_t)event;
}

bool event_event_cancel(uint64_t id) {
	return list_remove_data(events, (void*)id);
}

void event_trigger(int event_type, void* event) {
	Trigger* trigger = malloc(sizeof(Trigger));
	trigger->event_type = event_type;
	trigger->event = event;
	
	list_add(triggers, trigger);
}

void event_once(void(*func)(void*), void* context) {
	Event* event = malloc(sizeof(Event));
	event->func = func;
	event->context = context;
	
	list_add(oevents, event);
}

uint64_t event_tevent(bool(*func)(void*), void* context, uint64_t delay, uint64_t period) {
	TimedEvent* event = malloc(sizeof(TimedEvent));
	event->func = func;
	event->context = context;
	// TODO: Convert not using double
	uint64_t tsc = cpu_tsc();
	event->time = tsc + cpu_frequency * ((double)delay / 1000000000.0);
	event->period = cpu_frequency * ((double)period / 1000000000.0);
	
	int index = list_index_of(tevents, (void*)event->time, get_first_bigger);
	list_add_at(tevents, index, event);
	
	return (uint64_t)event;
}

bool event_tevent_cancel(uint64_t id) {
	free((void*)id);
	return list_remove_data(tevents, (void*)id);
}

void event_idle(void(*func)(void*), void* context) {
	Event* event = malloc(sizeof(Event));
	event->func = func;
	event->context = context;
	
	list_add(idles, event);
}

void event_busy(void(*func)(void*), void* context) {
	Event* event = malloc(sizeof(Event));
	event->func = func;
	event->context = context;
	
	list_add(bevents, event);
}
