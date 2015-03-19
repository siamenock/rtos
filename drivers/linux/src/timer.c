#include <linux/timer.h>
#include <util/event.h>
#include <string.h>

void init_timer(struct timer_list *timer) {
	bzero(timer, sizeof(struct timer_list));
	timer->id = (uint64_t)-1;
}

void add_timer(struct timer_list *timer) {
	bool fn(struct timer_list* timer) {
		timer->function(timer->data);
		
		return true;
	}
	
	timer->id = event_timer_add((void*)fn, (void*)timer, timer->expires, timer->expires);
}

int mod_timer(struct timer_list *timer, unsigned long expires) {
	if(timer->id == (uint64_t)-1) {
		timer->expires = expires;
		add_timer(timer);
	} else if(timer->expires != expires) {
		event_timer_remove(timer->id);
		timer->expires = expires;
		add_timer(timer);
	}
	
	return 0;
}

void del_timer(struct timer_list *timer) {
	event_timer_remove(timer->id);
}

