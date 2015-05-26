#ifndef __LINUX_NOTIFIER_H__
#define __LINUX_NOTIFIER_H__

#include <linux/compiler.h> // included by over 2 upper header 

struct notifier_block;

typedef	int (*notifier_fn_t)(struct notifier_block *nb, unsigned long action, void *data);

struct notifier_block {
	notifier_fn_t notifier_call;
	struct notifier_block __rcu *next;
	int priority;
};

#endif /* __LINUX_NOTIFIER_H__ */
