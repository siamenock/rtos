#ifndef __LINUX_SYSFS_H__
#define __LINUX_SYSFS_H__

#include <linux/types.h>
#include <linux/stringify.h>
#include <linux/stat.h>

#define __ATTR_RO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IRUGO },	\
	.show	= _name##_show,						\
}

#define __ATTRIBUTE_GROUPS(_name)				\
static const struct attribute_group *_name##_groups[] = {	\
	&_name##_group,						\
	NULL,							\
}

#define ATTRIBUTE_GROUPS(_name)					\
static const struct attribute_group _name##_group = {		\
	.attrs = _name##_attrs,					\
};								\
__ATTRIBUTE_GROUPS(_name)

struct attribute {
	const char	*name;
	umode_t		mode;
};

struct attribute_group {
	const char		*name;
//	umode_t			(*is_visible)(struct kobject *,
//					struct attribute *, int);
	struct attribute	**attrs;
//	struct bin_attribute	**bin_attrs;
};

#endif /* __LINUX_SYSFS_H__ */
