#include <linux/net.h>
#include <linux/printk.h>

int net_ratelimit() {
	printf("netif: ratelimit\n");
	return 1;
}
