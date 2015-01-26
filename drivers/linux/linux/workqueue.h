#ifndef __LINUX_WORKQUEUE_H__
#define __LINUX_WORKQUEUE_H__

#include <linux/types.h>

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

bool queue_work(struct workqueue_struct *wq, struct work_struct *work);

struct workqueue_struct* alloc_workqueue(const char *fmt, unsigned int flags, int max_active, const char *lock_name);

#define create_workqueue(name) alloc_workqueue("%s", WQ_MEM_RECLAIM, 1, (name))

#define DECLARE_WORK(n, f)                                              \
	struct work_struct n = { 0, f }

#define INIT_WORK(_work, _func) 		\
	do {					\
		(_work)->data = 0;		\
		(_work)->func = (_func);	\
	} while(0);
		

#endif /* __LINUX_WORKQUEUE_H__ */

