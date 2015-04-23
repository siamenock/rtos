#ifndef __LINUX_SKBUFF_H__
#define __LINUX_SKBUFF_H__

#include <linux/types.h>
#include <linux/net.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>

#define MAX_SKB_FRAGS			1
#define NET_SKB_PAD			32 
#define NET_IP_ALIGN			2

#define CHECKSUM_NONE			0
#define CHECKSUM_UNNECESSARY		1
#define CHECKSUM_COMPLETE		2
#define CHECKSUM_PARTIAL		3

#define dev_kfree_skb(a)        consume_skb(a)

typedef unsigned char *sk_buff_data_t;

struct sk_buff {
	union {
		struct {
			/* These two members must be first. */
			struct sk_buff		*next;
			struct sk_buff		*prev;

			union {
				//				ktime_t		tstamp;
				//				struct skb_mstamp skb_mstamp;
			};
		};
		//	struct rb_node	rbnode; /* used in netem & tcp stack */
	};

	struct net_device*	dev;

	__be16			protocol;

	unsigned int		len;
	unsigned int 		tail;
	sk_buff_data_t		end;
	unsigned char*		data;
	unsigned int		data_len; 

	__u8			ip_summed:2;

	unsigned char*		head;  
};

struct skb_shared_info {
	unsigned char		nr_frags;
	__u8			tx_flags;
	unsigned short		gso_size;
	unsigned short		gso_segs;
	unsigned short		gso_type;
	struct sk_buff		*frag_list;
	u32			tskey;
	__be32			lip6_frag_id;
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
//

#define skb_shinfo(SKB)	((struct skb_shared_info *)(skb_end_pointer(SKB)))

static inline unsigned char *skb_end_pointer(const struct sk_buff *skb)
{
	return NULL;
	//	return skb->head + skb->end;
}

static inline int skb_tailroom(const struct sk_buff *skb)
{
	return 0; //skb_is_nonlinear(skb) ? 0 : skb->end - skb->tail;
}
void skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page, int off,
		int size, unsigned int truesize);

void skb_trim(struct sk_buff *skb, unsigned int len);

static inline bool skb_can_coalesce(struct sk_buff *skb, int i,
		const struct page *page, int off)
{
	//	if (i) {
	//		const struct skb_frag_struct *frag = &skb_shinfo(skb)->frags[i - 1];
	//
	//		return page == skb_frag_page(frag) &&
	//		       off == frag->page_offset + skb_frag_size(frag);
	//	}
	//	return false;
	return true;
}

bool skb_partial_csum_set(struct sk_buff *skb, u16 start, u16 off);
struct sk_buff *__netdev_alloc_skb_ip_align(struct net_device *dev,
		unsigned int length, gfp_t gfp);
int skb_to_sgvec(struct sk_buff *skb, struct scatterlist *sg, int offset, int len);
#endif /* __LINUX_SKBUFF_H__ */

