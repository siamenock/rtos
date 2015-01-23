#ifndef __LINUX_SKBUFF_H__
#define __LINUX_SKBUFF_H__

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/netdevice.h>

#define CHECKSUM_NONE		0
#define CHECKSUM_UNNECESSARY	1
#define CHECKSUM_COMPLETE	2
#define CHECKSUM_PARTIAL	3

typedef unsigned char *sk_buff_data_t;

struct sk_buff {
	struct net_device*	dev;
	
	__be16			protocol;
	
	unsigned int		len;
	sk_buff_data_t		tail;
	sk_buff_data_t		end;
	unsigned char*		data;

	__u8			ip_summed:2;
};

struct sk_buff *dev_alloc_skb(unsigned int length);
int skb_pad(struct sk_buff *skb, int pad);
unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
void skb_reserve(struct sk_buff *skb, int len);

#endif /* __LINUX_SKBUFF_H__ */

