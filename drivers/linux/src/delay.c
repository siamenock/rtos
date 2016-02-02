#include <linux/delay.h>
#include <timer.h>

void udelay(unsigned int d) {
	timer_uwait(d);
}

void mdelay(unsigned int d) {
	timer_mwait(d);
}

void msleep(unsigned int d) {
	timer_mwait(d);
}	

