#include <linux/delay.h>
#include <timer.h>

void udelay(unsigned int d) {
	time_uwait(d);
}

void mdelay(unsigned int d) {
	time_mwait(d);
}

void msleep(unsigned int d) {
	time_mwait(d);
}	

