#include "asm.h"
#include "port.h"
#include "rtc.h"


#define RTC_ADDRESS		0x70
#define RTC_DATA		0x71

#define RTC_ADDR_SECOND		0x00
#define RTC_ADDR_MINUTE		0x02
#define RTC_ADDR_HOUR		0x04

#define RTC_ADDR_WEEK		0x06
#define RTC_ADDR_DATE		0x07
#define RTC_ADDR_MONTH		0x08
#define RTC_ADDR_YEAR		0x09

#define BCD(v)	(((((v) >> 4) & 0x0f) * 10) + ((v) & 0x0f))

// 0xff0000: hour, 0xff00: minute, 0xff: second
uint32_t rtc_time() {
	uint32_t result = 0;
	uint32_t tmp;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_HOUR);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 16;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_MINUTE);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 8;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_SECOND);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 0;
	
	return result;
}

// 0xff000000: year, 0xff0000: month, 0xff00: day of month, 0xff: day of week
uint32_t rtc_date() {
	uint32_t result = 0;
	uint32_t tmp;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_YEAR);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 24;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_MONTH);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 16;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_DATE);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 8;
	
	port_out8(RTC_ADDRESS, RTC_ADDR_WEEK);
	tmp = port_in8(RTC_DATA);
	result |= BCD(tmp) << 0;
	
	return result;
}
