#ifndef __LINUX_JIFFIES_H__
#define __LINUX_JIFFIES_H__

#include <linux/types.h>

#define MAX_JIFFY_OFFSET ((LONG_MAX >> 1)-1)

#define HZ		1000000
#define USER_HZ		1024
#define	CLOCKS_PER_SEC	USER_HZ	/* frequency at which times() counts */

unsigned long msecs_to_jiffies(const unsigned int m);
unsigned int jiffies_to_msecs(const unsigned long j);
unsigned long usecs_to_jiffies(const unsigned int u);
unsigned int jiffies_to_usecs(const unsigned long j);
unsigned long round_jiffies(unsigned long j);

extern uint64_t jiffies;

#define time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)((b) - (a)) < 0))
#define time_before(a,b)	time_after(b,a)

#define time_is_before_jiffies(a) time_after(jiffies, a)
#define time_is_after_jiffies(a) time_before(jiffies, a)

#endif /* __LINUX_JIFFIES_H__ */

