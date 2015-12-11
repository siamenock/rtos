#ifndef __LINUX_ETHERDEVICE_H__
#define __LINUX_ETHERDEVICE_H__

#include <stdbool.h>
#include <linux/types.h>
#include <linux/netdevice.h>

bool is_valid_ether_addr(const u8 *addr);
__be16 eth_type_trans(struct sk_buff *skb, struct net_device *dev);
struct net_device* alloc_etherdev(int sizeof_priv); 
int eth_validate_addr(struct net_device *dev);

static inline bool is_multicast_ether_addr(const u8 *addr)
{
	return 0x01 & addr[0];
}

#endif /* __LINUX_ETHERDEVICE_H__ */

