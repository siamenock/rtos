#ifndef __ASM_BUG_H__
#define __ASM_BUG_H__

#include <linux/printk.h>

#define BUG_ON(condition)	 		\
	do {					\
		if(condition) {			\
			printf("Bug!\n");	\
			while(1) asm("hlt");	\
		}				\
	} while(0)

#define BUG()			BUG_ON(true)

#endif /* __ASM_BUG_H__ */
