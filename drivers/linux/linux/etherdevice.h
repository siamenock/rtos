#ifndef __LINUX_ETHERDEVICE_H__
#define __LINUX_ETHERDEVICE_H__

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

bool is_valid_ether_addr(const u8 *addr);
__be16 eth_type_trans(struct sk_buff *skb, struct net_device *dev);

#endif /* __LINUX_ETHERDEVICE_H__ */

