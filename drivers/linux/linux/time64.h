#ifndef __LINUX_TIME64_H__
#define __LINUX_TIME64_H__

#include <linux/time.h>

typedef int64_t time64_t;

struct timespec64 {
	time64_t tv_sec;
	long tv_nsec;
};

void set_normalized_timespec64(struct timespec64 *ts, time_t sec, int64_t nsec);

static inline struct timespec64 timespec64_add(struct timespec64 lhs, struct timespec64 rhs) {
	struct timespec64 ts_delta;
	set_normalized_timespec64(&ts_delta, lhs.tv_sec + rhs.tv_sec, lhs.tv_nsec + rhs.tv_nsec);
	return ts_delta;
}

struct timespec64 ns_to_timespec64(const int64_t nsec);

#endif /* __LINUX_TIME64_H__ */
