#ifndef __LINUX_NETDEVICE_H__
#define __LINUX_NETDEVICE_H__

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <linux/jiffies.h>
#include <uapi/linux/netdevice.h>

enum netdev_tx {
	__NETDEV_TX_MIN  = INT_MIN,     /* make sure enum is signed */
	NETDEV_TX_OK     = 0x00,        /* driver took care of packet */
	NETDEV_TX_BUSY   = 0x10,        /* driver tx path was busy*/
	NETDEV_TX_LOCKED = 0x20,        /* driver tx lock was already taken */
};

typedef enum netdev_tx netdev_tx_t;

struct net_device_stats {
	unsigned long   rx_packets;
	unsigned long   tx_packets;
	unsigned long   rx_bytes;
	unsigned long   tx_bytes;
	unsigned long   rx_errors;
	unsigned long   tx_errors;
	unsigned long   rx_dropped;
	unsigned long   tx_dropped;
	unsigned long   multicast;
	unsigned long   collisions;
	unsigned long   rx_length_errors;
	unsigned long   rx_over_errors;
	unsigned long   rx_crc_errors;
	unsigned long   rx_frame_errors;
	unsigned long   rx_fifo_errors;
	unsigned long   rx_missed_errors;
	unsigned long   tx_aborted_errors;
	unsigned long   tx_carrier_errors;
	unsigned long   tx_fifo_errors;
	unsigned long   tx_heartbeat_errors;
	unsigned long   tx_window_errors;
	unsigned long   rx_compressed;
	unsigned long   tx_compressed;
};

struct net_device;

struct net_device_ops {
	int				(*ndo_open)(struct net_device *dev);
	int				(*ndo_stop)(struct net_device *dev);
	netdev_tx_t			(*ndo_start_xmit) (struct sk_buff *skb, struct net_device *dev);
	int				(*ndo_set_mac_address)(struct net_device *dev, void *addr);
	int				(*ndo_do_ioctl)(struct net_device *dev, struct ifreq *ifr, int cmd);
	struct net_device_stats*	(*ndo_get_stats)(struct net_device *dev);
};

struct net_device {
	int				irq;
	
	struct net_device_stats		stats;
	unsigned char*			dev_addr;
	const struct net_device_ops*	netdev_ops;
	int				watchdog_timeo;
	unsigned int			mtu;
};

struct net_device *alloc_netdev_mqs(int sizeof_priv, const char *name,
		void (*setup)(struct net_device *),
		unsigned int txqs, unsigned int rxqs);

#define alloc_netdev(sizeof_priv, name, setup) \
        alloc_netdev_mqs(sizeof_priv, name, setup, 1, 1)

void free_netdev(struct net_device *dev);

void ether_setup(struct net_device *dev);
void *netdev_priv(const struct net_device *dev);

void dev_kfree_skb_any(struct sk_buff *skb);

int register_netdev(struct net_device *dev);
void unregister_netdev(struct net_device *dev);

void netif_start_queue(struct net_device *dev);
void netif_stop_queue(struct net_device *dev);
bool netif_queue_stopped(const struct net_device *dev);
int netif_receive_skb(struct sk_buff *skb);
void netif_wake_queue(struct net_device *dev);

#endif /* __LINUX_NETDEVICE_H__ */

