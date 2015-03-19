#ifndef __LINUX_PRINTK_H__
#define __LINUX_PRINTK_H__

#include <linux/types.h>

int printf(const char *format, ...);

#define printk		printf

#define KERN_ERR	"KernErr"
#define KERN_INFO	"KernInfo"
#define KERN_ALERT	"KernAlert"

#endif /* __LINUX_PRINTK_H__ */

