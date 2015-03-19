#ifndef __LINUX_SKBUFF_H__
#define __LINUX_SKBUFF_H__

#include <linux/types.h>

#define MAX_SKB_FRAGS			1
#define NET_SKB_PAD				32 

#define CHECKSUM_NONE			0
#define CHECKSUM_UNNECESSARY	1
#define CHECKSUM_COMPLETE		2
#define CHECKSUM_PARTIAL		3

#define dev_kfree_skb(a)        consume_skb(a)

typedef unsigned char *sk_buff_data_t;

struct sk_buff {
	struct net_device*	dev;
	
	__be16				protocol;
	
	unsigned int		len;
	unsigned int 		tail;
	sk_buff_data_t		end;
	unsigned char*		data;
	unsigned int		data_len; 

	__u8				ip_summed:2;
	 
	unsigned char*		head;  
};

struct skb_shared_info {
	unsigned char	nr_frags;
	__u8			tx_flags;
	unsigned short	gso_size;
	unsigned short	gso_segs;
	unsigned short  gso_type;
	struct sk_buff	*frag_list;
	u32				tskey;
	__be32          ip6_frag_id;
	atomic_t		dataref;
	void *			destructor_arg;
};

unsigned int skb_headlen(const struct sk_buff *skb);
struct sk_buff *netdev_alloc_skb_ip_align(struct net_device* dev, int size);
void consume_skb(struct sk_buff *skb);
int skb_padto(struct sk_buff *skb, unsigned int len);
struct sk_buff* __vlan_hwaccel_put_tag(struct sk_buff* skb, __be16 vlan_proto, u16 vlan_tci);
struct sk_buff *dev_alloc_skb(unsigned int length);
int skb_pad(struct sk_buff *skb, int pad);
unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
void skb_reserve(struct sk_buff *skb, int len);

#endif /* __LINUX_SKBUFF_H__ */

