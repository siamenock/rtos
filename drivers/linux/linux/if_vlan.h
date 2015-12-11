#ifndef __LINUX_IF_VLAN_H__
#define __LINUX_IF_VLAN_H__

#include <linux/skbuff.h>
#define VLAN_HLEN 	4
#define VLAN_N_VID	4096

static inline __be16 vlan_get_protocol(struct sk_buff *skb)
{
	/* Start of GurumNetworks modification
	return __vlan_get_protocol(skb, skb->protocol, NULL);
	End of GurumNetworks modification */
	return skb->protocol;
}

#endif /* __IF_VLAN_H__ */
