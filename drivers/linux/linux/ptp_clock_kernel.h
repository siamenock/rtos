#ifndef __LINUX_PTP_CLOCK_KERNEL_H__
#define __LINUX_PTP_CLOCK_KERNEL_H__

#include <linux/time64.h>

enum ptp_pin_function {
	PTP_PF_NONE,
	PTP_PF_EXTTS,
	PTP_PF_PEROUT,
	PTP_PF_PHYSYNC,
};

struct ptp_clock_info {
	int (*gettime64)(struct ptp_clock_info *ptp, struct timespec64 *ts);
};

struct ptp_pin_desc {
	unsigned int index;
	unsigned int func;
};

struct ptp_clock {
};

struct ptp_extts_request {
	uint32_t index;
};

struct ptp_clock_time {
	int64_t sec;
	uint32_t nsec;
	uint32_t reserved;
};

struct ptp_perout_request {
	struct ptp_clock_time start;
	struct ptp_clock_time period;
	uint32_t index;
};

struct ptp_clock_request {
	enum {
		PTP_CLK_REQ_EXTTS,
		PTP_CLK_REQ_PEROUT,
		PTP_CLK_REQ_PPS,
	} type;
	union {
		struct ptp_extts_request extts;
		struct ptp_perout_request perout;
	};
};

int ptp_find_pin(struct ptp_clock *ptp, enum ptp_pin_function, uint32_t chan);

#endif /* __LINUX_PTP_CLOCK_KERNEL_H__ */
