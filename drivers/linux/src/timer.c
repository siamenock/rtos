#include <linux/timer.h>
#include <linux/printk.h>
#include <util/event.h>
#include <string.h>

void init_timer(struct timer_list *timer) {
	bzero(timer, sizeof(struct timer_list));
	timer->id = (uint64_t)-1;
}

static bool tick(struct timer_list* timer) {
	timer->function(timer->data);
	return true;
}

void add_timer(struct timer_list *timer) {
	timer->id = event_timer_add((void*)tick, (void*)timer, timer->expires, timer->expires);
}

int mod_timer(struct timer_list *timer, unsigned long expires) {
	if(!event_timer_update(timer->id, expires)) {
		timer->expires = expires;
		add_timer(timer);
	}

	return 0;
}

void del_timer(struct timer_list *timer) {
	event_timer_remove(timer->id);
}

void setup_timer(struct timer_list *timer, void(*function)(unsigned long), unsigned long data) {
	init_timer(timer);
	timer->function = function;
	timer->data = data;
	add_timer(timer);
}
