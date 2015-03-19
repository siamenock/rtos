#include <linux/delay.h>
#include <cpu.h>

void udelay(unsigned int d) {
	cpu_uwait(d);
}

void mdelay(unsigned int d) {
	cpu_mwait(d);
}

void msleep(unsigned int d) {
	cpu_mwait(d);
}	

