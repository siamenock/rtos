#include <linux/etherdevice.h>
#include <gmalloc.h>
#include <string.h>

// include/linux/etherdevice.h
static inline bool is_zero_ether_addr(const u8 *addr)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
	return ((*(const u32 *)addr) | (*(const u16 *)(addr + 4))) == 0;
#else
	return (*(const u16 *)(addr + 0) |
		*(const u16 *)(addr + 2) |
		*(const u16 *)(addr + 4)) == 0;
#endif
}

bool is_valid_ether_addr(const u8 *addr) {
	/* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
	 * explicitly check for it here. */
	return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}

int eth_validate_addr(struct net_device *dev) {
	if (!is_valid_ether_addr(dev->dev_addr))
		return -1;
	
	return 0;
}

__be16 eth_type_trans(struct sk_buff *skb, struct net_device *dev){ return 0; }

struct net_device* alloc_etherdev(int sizeof_priv) {
	struct net_device* dev = gmalloc(sizeof(struct net_device));
	bzero(dev, sizeof(struct net_device));
//	memcpy(dev->name, "eth0", 5);
	dev->mtu = 1500;
	dev->addr_len = ETH_ALEN;
	dev->priv = gmalloc(sizeof_priv);
	bzero(dev->priv, sizeof_priv);
	
	return dev;
}
