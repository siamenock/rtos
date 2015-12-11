#ifndef __LINUX_KTIME_H__
#define __LINUX_KTIME_H__

#include <linux/time.h>

#define KTIME_MAX		((int64_t)~((uint64_t)1 << 63))
#define KTIME_SEC_MAX		(KTIME_MAX / NSEC_PER_SEC)
static inline int64_t ktime_set(const int64_t secs, const uint64_t nsecs) {
	if(secs >= KTIME_SEC_MAX)
		return KTIME_MAX;

	return secs * NSEC_PER_SEC + (int64_t)nsecs;
}
#endif /* __LINUX_KTIME_H__ */
