#ifndef __LINUX_WORKQUEUE_H__
#define __LINUX_WORKQUEUE_H__

#include <stdbool.h> 
#include <linux/types.h>

#define NR_CPUS		1 //CONFIG_NR_CPUS  // Maximum supported processors
#define work_data_bits(work) ((unsigned long *)(&(work)->data)) //
#define create_workqueue(name) alloc_workqueue("%s", WQ_MEM_RECLAIM, 1, (name))

#define DECLARE_WORK(n, f)                                              \
	struct work_struct n = { 0, f }

#define INIT_WORK(_work, _func) 		\
	do {					\
		(_work)->data = 0;		\
		(_work)->func = (_func);	\
	} while(0);

enum {
	WORK_STRUCT_PENDING_BIT	= 0,	/* work item is pending execution */
	/* not bound to any CPU, prefer the local CPU */
	WORK_CPU_UNBOUND	= NR_CPUS,
}; 

enum {
	WQ_FREEZABLE	= 1 << 2,
	WQ_MEM_RECLAIM	= 1 << 3
};

struct workqueue_struct;
struct work_struct;

typedef void (*work_func_t)(struct work_struct* work);

struct work_struct {
	long		data;
	work_func_t	func;
};

struct delayed_work {
	struct work_struct work;
//	struct timer_list timer;

	/* target workqueue and CPU ->timer uses to queue ->work */
	struct workqueue_struct *wq;
	int cpu;
};

struct workqueue_struct* alloc_workqueue(const char *fmt, unsigned int flags, int max_active, const char *lock_name);
bool queue_work(struct workqueue_struct *wq, struct work_struct *work);
bool schedule_work(struct work_struct *work);
bool schedule_delayed_work(struct delayed_work *work, clock_t delay);
void cancel_work_sync(struct work_struct *work);
void synchronize_sched();

#endif /* __LINUX_WORKQUEUE_H__ */

