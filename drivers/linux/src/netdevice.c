#include <linux/netdevice.h>
#include <linux/netdev_features.h>
#include <linux/if.h>
#include <linux/if_arp.h> 
#include <linux/printk.h>
#include <linux/string.h>
#include <gmalloc.h>

struct net_device *alloc_netdev_mqs(int sizeof_priv, const char *name,
				void (*setup)(struct net_device *),
						unsigned int txqs, unsigned int rxqs) { 
	return NULL; 
}

void free_netdev(struct net_device *dev) {
	if(dev) {
		gfree(dev->priv);
		gfree(dev);
	}
}

void netif_device_detach(struct net_device *dev) {
	printf("netif: detatch\n");
}

void netif_carrier_on(struct net_device *dev) {
//	printf("netif: carrier on\n");
	if(!test_and_clear_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
	}
}

void netif_carrier_off(struct net_device *dev) {
//	printf("netif: carrier off\n");
	if(!test_and_set_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
	}
}

void netif_tx_start_all_queues(struct net_device *dev) {
	unsigned int i;

	for (i = 0; i < dev->num_tx_queues; i++) {
		struct netdev_queue *txq = netdev_get_tx_queue(dev, i);
		txq->state = 1;
		netif_tx_start_queue(txq);
	}
}

void netif_tx_stop_all_queues(struct net_device *dev) {
	uint32_t i;
	for(i = 0; i < dev->num_tx_queues; i++) {
		struct netdev_queue *txq = netdev_get_tx_queue(dev, i);
		txq->state = 0;
		netif_tx_stop_queue(txq);
	}
}

int netif_running(const struct net_device *dev) {
	return 1;//dev->tx_queue == 1;
}

void ether_setup(struct net_device *dev)
{
//	dev->header_ops			= &eth_header_ops;

	dev->type				= ARPHRD_ETHER;
	dev->hard_header_len 	= ETH_HLEN;
	dev->mtu				= ETH_DATA_LEN;
	dev->addr_len			= ETH_ALEN;
	dev->tx_queue_len		= 1000;	/* Ethernet wants good queues */
	dev->flags				= IFF_BROADCAST|IFF_MULTICAST;
	dev->priv_flags			|= IFF_TX_SKB_SHARING;

	memset(dev->broadcast, 0xFF, ETH_ALEN);

}

void *netdev_priv(const struct net_device *dev) { 
	return dev->priv;
}

netdev_features_t netdev_fix_features(struct net_device *dev, netdev_features_t features) {
	/* Fix illegal checksum combinations */
	if ((features & NETIF_F_HW_CSUM) &&
	    (features & (NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM))) {
		netdev_warn(dev, "mixed HW and IP checksum settings.\n");
		features &= ~(NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM);
	}

	/* TSO requires that SG is present as well. */
	if ((features & NETIF_F_ALL_TSO) && !(features & NETIF_F_SG)) {
		netdev_dbg(dev, "Dropping TSO features since no SG feature.\n");
		features &= ~NETIF_F_ALL_TSO;
	}

	if ((features & NETIF_F_TSO) && !(features & NETIF_F_HW_CSUM) &&
					!(features & NETIF_F_IP_CSUM)) {
		netdev_dbg(dev, "Dropping TSO features since no CSUM feature.\n");
		features &= ~NETIF_F_TSO;
		features &= ~NETIF_F_TSO_ECN;
	}

	if ((features & NETIF_F_TSO6) && !(features & NETIF_F_HW_CSUM) &&
					 !(features & NETIF_F_IPV6_CSUM)) {
		netdev_dbg(dev, "Dropping TSO6 features since no CSUM feature.\n");
		features &= ~NETIF_F_TSO6;
	}

	/* TSO ECN requires that TSO is present as well. */
	if ((features & NETIF_F_ALL_TSO) == NETIF_F_TSO_ECN)
		features &= ~NETIF_F_TSO_ECN;

	/* Software GSO depends on SG. */
	if ((features & NETIF_F_GSO) && !(features & NETIF_F_SG)) {
		netdev_dbg(dev, "Dropping NETIF_F_GSO since no SG feature.\n");
		features &= ~NETIF_F_GSO;
	}

	/* UFO needs SG and checksumming */
	if (features & NETIF_F_UFO) {
		/* maybe split UFO into V4 and V6? */
		if (!((features & NETIF_F_GEN_CSUM) ||
		    (features & (NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM))
			    == (NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM))) {
			netdev_dbg(dev,
				"Dropping NETIF_F_UFO since no checksum offload features.\n");
			features &= ~NETIF_F_UFO;
		}

		if (!(features & NETIF_F_SG)) {
			netdev_dbg(dev,
				"Dropping NETIF_F_UFO since no NETIF_F_SG feature.\n");
			features &= ~NETIF_F_UFO;
		}
	}

	return features;
}

int __netdev_update_features(struct net_device *dev) {
	int err = 0;
	netdev_features_t features = dev->features;
	
	if(dev->netdev_ops->ndo_fix_features)
		features = dev->netdev_ops->ndo_fix_features(dev, features);
	
	features = netdev_fix_features(dev, features);
	
	if (dev->features == features)
		return 0;
	
	netdev_dbg(dev, "Features changed: %pNF -> %pNF\n", &dev->features, &features);
	
	if (dev->netdev_ops->ndo_set_features)
		err = dev->netdev_ops->ndo_set_features(dev, features);
	
	if (err < 0) {
		netdev_err(dev,
			"set_features() failed (%d); wanted %pNF, left %pNF\n",
			err, &features, &dev->features);
		return -1;
	}
	
	if (!err)
		dev->features = features;

	return 1;
}

/* Change event listener */
void netdev_features_change(struct net_device *dev) {
	// Do nothing
        //call_netdevice_notifiers(NETDEV_FEAT_CHANGE, dev);
}

void netdev_update_features(struct net_device *dev) {
        if (__netdev_update_features(dev))
                netdev_features_change(dev);
}

void netdev_change_features(struct net_device *dev) {
        __netdev_update_features(dev);
        netdev_features_change(dev);
}

void dev_kfree_skb_any(struct sk_buff *skb) {
	gfree(skb->data);
	gfree(skb);
}

int register_netdev(struct net_device *dev) { 
	printf("register netdev\n");
	return 0;
}

void unregister_netdev(struct net_device *dev) {
	printf("unregister netdev\n");
}

void netif_start_queue(struct net_device *dev) {
	printf("netif: start queue\n");
	dev->tx_queue = 1;
}

void netif_stop_queue(struct net_device *dev) {
	//printf("netif: stop queue\n");
	dev->tx_queue = 0;
}

int netif_queue_stopped(const struct net_device *dev) {
	return dev->tx_queue == 0;
}

int netif_receive_skb(struct sk_buff *skb) { 
	return 0;
}

void netif_wake_queue(struct net_device *dev) {
	dev->tx_queue = 1;
}

void netif_wake_subqueue(struct net_device *dev, u16 queue_index) {
	(&dev->_tx[queue_index])->state = 1;
}

int netif_set_real_num_tx_queues(struct net_device *dev, unsigned int txq) {
	return 0;
}

int netif_set_real_num_rx_queues(struct net_device *dev, unsigned int rxq) {
	return 0;
}

void napi_enable(struct napi_struct* n) {
//	printf("napi: enable\n");
	n->enabled = true;
}

void napi_disable(struct napi_struct* n) {
	printf("napi: disable\n");
	n->enabled = false;
}

void napi_gro_receive(struct napi_struct* n, struct sk_buff* buf) {
	printf("napi: gro receive: %d\n", buf->len);
	for(int i = 0; i < buf->len; i++)
		printf("%02x", buf->data[i]);
	// TODO: Received
}

void napi_complete(struct napi_struct *n) {
}

void napi_schedule(struct napi_struct* n) {
	printf("napi: schedule\n");
}

