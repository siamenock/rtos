#ifndef __LINUX_DEVICE_H__
#define __LINUX_DEVICE_H__

#include <packetngin/device.h>
#include <linux/types.h>
#include <linux/sysfs.h>

#define DEVICE_ATTR_RO(_name)	\
	struct device_attribute dev_attr_##_name = __ATTR_RO(_name)

#define module_driver(__driver, __register, __unregister, ...) \
int __driver##_init(void) \
{ \
	return __register(&(__driver) , ##__VA_ARGS__); \
} \
module_init(__driver##_init); \
void __driver##_exit(void) \
{ \
	__unregister(&(__driver) , ##__VA_ARGS__); \
} \
module_exit(__driver##_exit);

#define dev_info(dev, fmt, args...)			printf("dev_info: " fmt, ##args)

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

struct device_driver {
	const char              *name;
	struct bus_type         *bus;

	struct module           *owner;
	const char              *mod_name;      /* used for built-in modules */
//	bool suppress_bind_attrs;       /* disables bind/unbind via sysfs */
//	const struct of_device_id       *of_match_table;
//	const struct acpi_device_id     *acpi_match_table;

	int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
	void (*shutdown) (struct device *dev);
//	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
//	const struct attribute_group **groups;
//	const struct dev_pm_ops *pm;
//	struct driver_private *p;
};

struct device_attribute {
	struct attribute	attr;
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
};

struct bus_type {
	const char		*name;
	const char		*dev_name;
	struct device		*dev_root;
	struct device_attribute	*dev_attrs;	/* use dev_groups instead */
//	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
//	const struct attribute_group **drv_groups;

	int (*match)(struct device *dev, struct device_driver *drv);
//	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	int (*remove)(struct device *dev);
	void (*shutdown)(struct device *dev);

	int (*online)(struct device *dev);
	int (*offline)(struct device *dev);

//	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);

//	const struct dev_pm_ops *pm;

//	const struct iommu_ops *iommu_ops;

//	struct subsys_private *p;
//	struct lock_class_key lock_key;
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

