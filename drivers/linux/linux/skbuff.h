#ifndef __LINUX_SKBUFF_H__
#define __LINUX_SKBUFF_H__

#include <linux/types.h>
#include <linux/net.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/atomic.h>

#define MAX_SKB_FRAGS			1
#define NET_SKB_PAD			32 
#define NET_IP_ALIGN			2

#define CHECKSUM_NONE			0
#define CHECKSUM_UNNECESSARY		1
#define CHECKSUM_COMPLETE		2
#define CHECKSUM_PARTIAL		3

#define dev_kfree_skb(a)        consume_skb(a)

/* Definitions for tx_flags in struct skb_shared_info */
enum {
	/* generate hardware time stamp */
	SKBTX_HW_TSTAMP = 1 << 0,

	/* generate software time stamp when queueing packet to NIC */
	SKBTX_SW_TSTAMP = 1 << 1,

	/* device driver is going to provide hardware time stamp */
	SKBTX_IN_PROGRESS = 1 << 2,

	/* device driver supports TX zero-copy buffers */
	SKBTX_DEV_ZEROCOPY = 1 << 3,

	/* generate wifi status information (where possible) */
	SKBTX_WIFI_STATUS = 1 << 4,

	/* This indicates at least one fragment might be overwritten
	 * (as in vmsplice(), sendfile() ...)
	 * If we need to compute a TX checksum, we'll need to copy
	 * all frags to avoid possible bad checksum
	 */
	SKBTX_SHARED_FRAG = 1 << 5,

	/* generate software time stamp when entering packet scheduling */
	SKBTX_SCHED_TSTAMP = 1 << 6,

	/* generate software timestamp on peer data acknowledgment */
	SKBTX_ACK_TSTAMP = 1 << 7,
};

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

	__u16			network_header;
	__u16			queue_mapping;
	__u8			cloned:1,
				nohdr:1,
				fclone:2,
				peeked:1,
				head_frag:1,
				xmit_more:1;
	atomic64_t		users;
};

typedef struct skb_frag_struct skb_frag_t;

struct skb_frag_struct {
	struct {
		struct page *p;
	} page;
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
	__u32 page_offset;
	__u32 size;
#else
	__u16 page_offset;
	__u16 size;
#endif
};

struct skb_shared_hwtstamps {
	int64_t hwtstamp;
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
	atomic64_t		dataref;
	void *			destructor_arg;

	skb_frag_t		frags[MAX_SKB_FRAGS];
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

#define skb_shinfo(SKB)	((struct skb_shared_info *)(skb_end_pointer(SKB)))

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
	return skb->head + skb->network_header;
}

static inline unsigned char *skb_end_pointer(const struct sk_buff *skb)
{
	return NULL;
	//	return skb->head + skb->end;
}

static inline int skb_tailroom(const struct sk_buff *skb)
{
	return 0; //skb_is_nonlinear(skb) ? 0 : skb->end - skb->tail;
}

static inline bool skb_is_gso(const struct sk_buff *skb)
{
	return skb_shinfo(skb)->gso_size;
}

#define SKB_DATAREF_SHIFT 16
#define SKB_DATAREF_MASK ((1 << SKB_DATAREF_SHIFT) - 1)
static inline int skb_header_cloned(const struct sk_buff *skb)
{
	int dataref;

	if (!skb->cloned)
		return 0;

	dataref = atomic64_read(&skb_shinfo(skb)->dataref);
	dataref = (dataref & SKB_DATAREF_MASK) - (dataref >> SKB_DATAREF_SHIFT);
	return dataref != 1;
}

static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
	return skb->data - skb->head;
}

static inline int skb_cow_head(struct sk_buff *skb, unsigned int headroom)
{
//	int delta = 0;
//
//	if (headroom > skb_headroom(skb))
//		delta = headroom - skb_headroom(skb);
//
//	if (delta || skb_header_cloned(skb))
//		return pskb_expand_head(skb, ALIGN(delta, NET_SKB_PAD), 0,
//					GFP_ATOMIC);
	return 0;
}

static inline unsigned int skb_frag_size(const skb_frag_t *frag)
{
	return frag->size;
}

static inline dma_addr_t skb_frag_dma_map(struct device *dev,
					  const skb_frag_t *frag,
					  size_t offset, size_t size,
					  enum dma_data_direction dir)
{
	return (dma_addr_t)dma_map_page(dev, frag->page.p,
			    frag->page_offset + offset, size, dir);
}

static inline struct sk_buff *skb_get(struct sk_buff *skb)
{
	atomic64_inc(&skb->users);
	return skb;
}

void skb_trim(struct sk_buff *skb, unsigned int len); 
bool skb_partial_csum_set(struct sk_buff *skb, u16 start, u16 off);
int skb_to_sgvec(struct sk_buff *skb, struct scatterlist *sg, int offset, int len);
struct sk_buff *__netdev_alloc_skb_ip_align(struct net_device *dev, unsigned int length, gfp_t gfp);
void skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page, int off, int size, unsigned int truesize);

static inline bool skb_can_coalesce(struct sk_buff *skb, int i, const struct page *page, int off) {
	//	if (i) {
	//		const struct skb_frag_struct *frag = &skb_shinfo(skb)->frags[i - 1];
	//
	//		return page == skb_frag_page(frag) &&
	//		       off == frag->page_offset + skb_frag_size(frag);
	//	}
	//	return false;
	return true;
}


#endif /* __LINUX_SKBUFF_H__ */

