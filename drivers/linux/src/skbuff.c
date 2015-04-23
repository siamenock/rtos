#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <stddef.h>
#include <gmalloc.h>

#define dev_kfree_skb(a)        consume_skb(a)

struct sk_buff *netdev_alloc_skb_ip_align(struct net_device* dev, int size) {
	struct sk_buff* skb = gmalloc(sizeof(struct sk_buff) + size);
	skb->dev = dev;
	skb->len = size;
	skb->head = skb->data;

	skb->data = gmalloc(size);
	return skb;
}

struct sk_buff *dev_alloc_skb(unsigned int length) {
	return netdev_alloc_skb_ip_align(NULL, length);
}

int skb_pad(struct sk_buff *skb, int pad) {
	return 0;
}

unsigned char *skb_put(struct sk_buff *skb, unsigned int len) {
	unsigned char* tmp = skb->head + skb->tail;
	
	skb->tail += len;
	skb->len += len;
	
	return tmp;
}

void skb_reserve(struct sk_buff *skb, int len) {
	skb->data += len;
	skb->tail += len;
}

void consume_skb(struct sk_buff *skb) {
	gfree(skb);
}

struct sk_buff* __vlan_hwaccel_put_tag(struct sk_buff* skb, __be16 vlan_proto, u16 vlan_tci) {
	// TODO: Implement it
	return skb;
}

unsigned int skb_headlen(const struct sk_buff *skb) {
        return skb->len - skb->data_len;
}

int skb_padto(struct sk_buff *skb, unsigned int len) {
	unsigned int size = skb->len;
	if (likely(size >= len))
		return 0;
	
	return 1;
}

static inline struct sk_buff *__netdev_alloc_skb(struct net_device *dev,
				   unsigned int length, gfp_t gfp_mask)
{
//	struct sk_buff *skb = NULL;
//	unsigned int fragsz = SKB_DATA_ALIGN(length + NET_SKB_PAD) +
//			      SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
//
//	if (fragsz <= PAGE_SIZE && !(gfp_mask & (__GFP_WAIT | GFP_DMA))) {
//		void *data;
//
//		if (sk_memalloc_socks())
//			gfp_mask |= __GFP_MEMALLOC;
//
//		data = __netdev_alloc_frag(fragsz, gfp_mask);
//
//		if (likely(data)) {
//			skb = build_skb(data, fragsz);
//			if (unlikely(!skb))
//				put_page(virt_to_head_page(data));
//		}
//	} else {
//		skb = __alloc_skb(length + NET_SKB_PAD, gfp_mask,
//				  SKB_ALLOC_RX, NUMA_NO_NODE);
//	}
//	if (likely(skb)) {
//		skb_reserve(skb, NET_SKB_PAD);
//		skb->dev = dev;
//	}
//	return skb;
	return NULL;
}

struct sk_buff *__netdev_alloc_skb_ip_align(struct net_device *dev,
		unsigned int length, gfp_t gfp)
{
	struct sk_buff *skb = __netdev_alloc_skb(dev, length + NET_IP_ALIGN, gfp);

	if (NET_IP_ALIGN && skb)
		skb_reserve(skb, NET_IP_ALIGN);
	return skb;
}

static int
__skb_to_sgvec(struct sk_buff *skb, struct scatterlist *sg, int offset, int len)
{
//	int start = skb_headlen(skb);
//	int i, copy = start - offset;
//	struct sk_buff *frag_iter;
//	int elt = 0;
//
//	if (copy > 0) {
//		if (copy > len)
//			copy = len;
//		sg_set_buf(sg, skb->data + offset, copy);
//		elt++;
//		if ((len -= copy) == 0)
//			return elt;
//		offset += copy;
//	}
//
//	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
//		int end;
//
//		WARN_ON(start > offset + len);
//
//		end = start + skb_frag_size(&skb_shinfo(skb)->frags[i]);
//		if ((copy = end - offset) > 0) {
//			skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
//
//			if (copy > len)
//				copy = len;
//			sg_set_page(&sg[elt], skb_frag_page(frag), copy,
//					frag->page_offset+offset-start);
//			elt++;
//			if (!(len -= copy))
//				return elt;
//			offset += copy;
//		}
//		start = end;
//	}
//
//	skb_walk_frags(skb, frag_iter) {
//		int end;
//
//		WARN_ON(start > offset + len);
//
//		end = start + frag_iter->len;
//		if ((copy = end - offset) > 0) {
//			if (copy > len)
//				copy = len;
//			elt += __skb_to_sgvec(frag_iter, sg+elt, offset - start,
//					      copy);
//			if ((len -= copy) == 0)
//				return elt;
//			offset += copy;
//		}
//		start = end;
//	}
//	BUG_ON(len);
//	return elt;
	return 0;
}

int skb_to_sgvec(struct sk_buff *skb, struct scatterlist *sg, int offset, int len)
{
	int nsg = __skb_to_sgvec(skb, sg, offset, len);

//	sg_mark_end(&sg[nsg - 1]);

	return nsg;
}
