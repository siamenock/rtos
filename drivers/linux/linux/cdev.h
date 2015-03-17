#ifndef __LINUX_CDEV_H__
#define __LINUX_CDEV_H__

#include <linux/types.h>

struct file_operations;

struct cdev {
	struct module*		owner;
	const struct file_operations*	ops; 
	
	dev_t dev; 
	unsigned int count; 
};

void cdev_init(struct cdev *, const struct file_operations *);
int cdev_add(struct cdev *, dev_t, unsigned);
void cdev_del(struct cdev *);

#endif /* __LINUX_CDEV_H__ */

