#ifndef __LINUX_DELAY_H__
#define __LINUX_DELAY_H__
 
void udelay(unsigned int d);
void mdelay(unsigned int d);
void msleep(unsigned int d);
void usleep_range(unsigned long min, unsigned long max);

#endif /* __LINUX_DELAY_H__ */

