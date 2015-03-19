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

