#ifndef __ARCH_OL_DRIVER__
#define __ARCH_OL_DRIVER__

#include <lwip/netif.h>

void ol_driver_poll(struct netif *netif);
err_t ol_driver_init(struct netif *netif);

#endif /* __ARCH_OL_DRIVER__ */
