#ifndef __LINUX_DEVICE_H__
#define __LINUX_DEVICE_H__

#include <packetngin/device.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/time.h>

struct class {
};

struct class* class_create(struct module *owner, const char *name);
void class_destroy(struct class *cls);
int class_register(struct class *class);
void class_unregister(struct class *class);

struct device *device_create(struct class *cls, 
	struct device *parent, dev_t devt, void *drvdata, 
	const char *fmt, ...);
	
struct attribute_group {
};

struct device *device_create_with_groups(struct class *cls,
	struct device *parent, dev_t devt, void *drvdata,
	const struct attribute_group **groups,
	const char *fmt, ...);

void device_destroy(struct class *cls, dev_t devt);

#endif /* __LINUX_DEVICE_H__ */

