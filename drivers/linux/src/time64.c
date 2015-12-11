#include <linux/time64.h>

void set_normalized_timespec64(struct timespec64 *ts, time_t sec, int64_t nsec)
{
	while (nsec >= NSEC_PER_SEC) {
		/*
		 * The following asm() prevents the compiler from
		 * optimising this loop into a modulo operation. See
		 * also __iter_div_u64_rem() in include/linux/time.h
		 */
		asm("" : "+rm"(nsec));
		nsec -= NSEC_PER_SEC;
		++sec;
	}
	while (nsec < 0) {
		asm("" : "+rm"(nsec));
		nsec += NSEC_PER_SEC;
		--sec;
	}
	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

struct timespec64 ns_to_timespec64(const int64_t nsec) {
	struct timespec64 ts;
	int64_t rem;

	if(!nsec)
		return (struct timespec64){0, 0};

	rem = nsec % NSEC_PER_SEC;
	ts.tv_sec = nsec / NSEC_PER_SEC;

	if(rem < 0) {
		ts.tv_sec--;
		rem += NSEC_PER_SEC;
	}
	ts.tv_nsec = rem;

	return ts;
}
