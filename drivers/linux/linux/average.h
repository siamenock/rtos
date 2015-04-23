#ifndef __LINUX_AVERAGE_H__
#define __LINUX_AVERAGE_H__

struct ewma {
	unsigned long internal;
	unsigned long factor;
	unsigned long weight;
};

#endif /* __LINUX_AVERAGE_H__ */
