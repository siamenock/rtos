#ifndef __LINUX_BUG_H__
#define __LINUX_BUG_H__

#include <asm/bug.h>
#include <linux/fs.h>

#define BUILD_BUG_ON_ZERO(e) (0)
#define BUILD_BUG_ON_NULL(e) ((void*)0)
#define BUILD_BUG_ON_INVALID(e) (0)
#define BUILD_BUG_ON_MSG(cond, msg) (0)
#define BUILD_BUG_ON(condition)	 		\
	do {					\
		if(condition) {			\
			printf("Bug!\n");	\
			while(1) asm("hlt");	\
		}				\
	} while(0)
#define BUILD_BUG() (0)

#define WARN_ON(condition) ({				\
		int __ret_warn_on = !!(condition);	\
		if(__ret_warn_on) {			\
			printf("Warn!\n");		\
			while(1) asm("hlt");		\
		}					\
})

#define WARN_ON_ONCE(condition)	WARN_ON(condition)

#endif /* __LINUX_BUG_H__ */
