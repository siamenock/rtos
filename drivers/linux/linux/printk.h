#ifndef __LINUX_PRINTK_H__
#define __LINUX_PRINTK_H__

#include <linux/types.h>

int printf(const char *format, ...);
int no_printk(const char *fmt, ...);

#define printk		printf

#define KERN_ERR	"KernErr"
#define KERN_INFO	"KernInfo"
#define KERN_ALERT	"KernAlert"
#define KERN_DEBUG	"KernDebug"

#ifdef DEBUG
#define pr_debug(fmt, ...) \
	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else	
#define pr_debug(fmt, ...) \
	no_printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#endif
#endif /* __LINUX_PRINTK_H__ */

