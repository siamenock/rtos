#include <linux/printk.h>

int no_printk(const char *fmt, ...) {
	return 0;
}
