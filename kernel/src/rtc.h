#ifndef __RTC_H__
#define __RTC_H__

#include <stdint.h>

#define RTC_HOUR(v)	(((v) >> 16) & 0xff)
#define RTC_MINUTE(v)	(((v) >> 8) & 0xff)
#define RTC_SECOND(v)	(((v) >> 0) & 0xff)

#define RTC_YEAR(v)	(((v) >> 24) & 0xff)
#define RTC_MONTH(v)	(((v) >> 16) & 0xff)
#define RTC_DATE(v)	(((v) >> 8) & 0xff)
#define RTC_WEEK(v)	(((v) >> 0) & 0xff)

uint32_t rtc_time();	// 0xff0000: hour, 0xff00: minute, 0xff: second
uint32_t rtc_date();	// 0xff000000: year, 0xff0000: month, 0xff00: day of month, 0xff: day of week

#endif /* __RTC_H__ */
