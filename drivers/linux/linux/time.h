#ifndef __LINUX_TIME_H__
#define __LINUX_TIME_H__

#include <linux/types.h> 
#define NSEC_PER_SEC	1000000000L

# define __isleap(year)	\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

struct tm {
	/*
	 * the number of seconds after the minute, normally in the range
	 * 0 to 59, but can be up to 60 to allow for leap seconds
	 */
	int tm_sec;
	/* the number of minutes after the hour, in the range 0 to 59*/
	int tm_min;
	/* the number of hours past midnight, in the range 0 to 23 */
	int tm_hour;
	/* the day of the month, in the range 1 to 31 */
	int tm_mday;
	/* the number of months since January, in the range 0 to 11 */
	int tm_mon;
	/* the number of years since 1900 */
	long tm_year;
	/* the number of days since Sunday, in the range 0 to 6 */
	int tm_wday;
	/* the number of days since January 1, in the range 0 to 365 */
	int tm_yday;
};

void time_to_tm(time_t totalsecs, int offset, struct tm *result);
struct timespec ns_to_timespec(const int64_t nsec);

static inline int64_t timespec_to_ns(const struct timespec *ts) {
	return ((int64_t)ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}
#endif /* __LINUX_TIME_H__ */

