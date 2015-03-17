#ifndef __LINUX_DEVICE_H__
#define __LINUX_DEVICE_H__

#include <packetngin/device.h>
#include <linux/types.h>

struct class {
	const char		*name;
	struct module		*owner;

	char *(*devnode)(struct device *dev, umode_t *mode);
	void (*class_release)(struct class *class);
	void (*dev_release)(struct device *dev);
	int (*resume)(struct device *dev);
	const void *(*namespace)(struct device *dev);

	const struct dev_pm_ops *pm;
};

struct class* class_create(struct module *owner, const char *name);
void class_destroy(struct class *cls);
int class_register(struct class *class);
void class_unregister(struct class *class);

struct device *device_create(struct class *cls, 
	struct device *parent, dev_t devt, void *drvdata, 
	const char *fmt, ...);
void device_destroy(struct class *cls, dev_t devt);

#endif /* __LINUX_DEVICE_H__ */

