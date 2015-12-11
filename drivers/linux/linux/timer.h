#ifndef __LINUX_TIMER_H__
#define __LINUX_TIMER_H__

#include <linux/types.h>
#include <timer.h>

#define del_timer_sync(timer)	del_timer(timer)

struct timer_list {
	uint64_t	id;
	
	unsigned long expires;
	void (*function)(unsigned long);
	unsigned long data;
};

void init_timer(struct timer_list *timer);
void add_timer(struct timer_list *timer);
int mod_timer(struct timer_list *timer, unsigned long expires);
void del_timer(struct timer_list *timer);
void setup_timer(struct timer_list *timer, void(*function)(unsigned long), unsigned long data);

#endif /* __LINUX_TIMER_H__ */

