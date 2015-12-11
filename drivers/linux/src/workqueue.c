#include <linux/workqueue.h>
#include <linux/compiler.h>
#include <asm/bitops.h>
#include <util/event.h>

struct workqueue_struct* alloc_workqueue(const char *fmt, unsigned int flags, int max_active, const char *lock_name) {
		return (void*)1;
}

bool queue_work(struct workqueue_struct *wq, struct work_struct *work) {
	return true;
}

bool schedule_work(struct work_struct *work) {
	event_busy_add((void*)work->func, work);
	return true;
}

bool schedule_delayed_work(struct delayed_work *work, clock_t delay) {
	event_timer_add((void*)(work->work).func, work, delay, delay);
	return true;
}
void cancel_work_sync(struct work_struct *work) {
}

void synchronize_sched() {
}

