#ifndef __LINUX_NETDEVICE_H__
#define __LINUX_NETDEVICE_H__

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/skbuff.h>
#include <linux/notifier.h>

#define MAX_ADDR_LEN	32

#define alloc_netdev(sizeof_priv, name, setup) \
	alloc_netdev_mqs(sizeof_priv, name, setup, 1, 1)

#define netif_err(priv, type, dev, fmt, args...)	printf("netif_err: " fmt, ##args)
#define netif_warn(priv, type, dev, fmt, args...)	printf("netif_warn: " fmt, ##args)
#define netif_info(priv, type, dev, fmt, args...)	printf("netif_info: " fmt, ##args)
#define netif_notice(priv, type, dev, fmt, args...)	printf("netif_notice: " fmt, ##args)

#define netif_msg_drv(p)        ((p)->msg_enable & NETIF_MSG_DRV)
#define netif_msg_probe(p)      ((p)->msg_enable & NETIF_MSG_PROBE)
#define netif_msg_link(p)       ((p)->msg_enable & NETIF_MSG_LINK)
#define netif_msg_timer(p)      ((p)->msg_enable & NETIF_MSG_TIMER)
#define netif_msg_ifdown(p)     ((p)->msg_enable & NETIF_MSG_IFDOWN)
#define netif_msg_ifup(p)       ((p)->msg_enable & NETIF_MSG_IFUP)
#define netif_msg_rx_err(p)     ((p)->msg_enable & NETIF_MSG_RX_ERR)
#define netif_msg_tx_err(p)     ((p)->msg_enable & NETIF_MSG_TX_ERR)
#define netif_msg_tx_queued(p)  ((p)->msg_enable & NETIF_MSG_TX_QUEUED)
#define netif_msg_intr(p)       ((p)->msg_enable & NETIF_MSG_INTR)
#define netif_msg_tx_done(p)    ((p)->msg_enable & NETIF_MSG_TX_DONE)
#define netif_msg_rx_status(p)  ((p)->msg_enable & NETIF_MSG_RX_STATUS)
#define netif_msg_pktdata(p)    ((p)->msg_enable & NETIF_MSG_PKTDATA)
#define netif_msg_hw(p)         ((p)->msg_enable & NETIF_MSG_HW)
#define netif_msg_wol(p)        ((p)->msg_enable & NETIF_MSG_WOL)
#define netif_msg_init(debug,value)	(((debug) == -1) ? (value) : (debug))

#define netdev_warn(dev, fmt, args...)			printf("netdev_warn: " fmt, ##args)
#define netdev_dbg(dev, fmt, args...)			printf("netdev_dbg: " fmt, ##args)
#define netdev_err(dev, fmt, args...)			printf("netdev_err: " fmt, ##args)

#define SET_NETDEV_DEV(net, pdev)	((net)->dev = (pdev))

#define NAPI_POLL_WEIGHT	64

enum {
	NETIF_MSG_DRV           = 0x0001,
	NETIF_MSG_PROBE         = 0x0002,
	NETIF_MSG_LINK          = 0x0004,
	NETIF_MSG_TIMER         = 0x0008,
	NETIF_MSG_IFDOWN        = 0x0010,
	NETIF_MSG_IFUP          = 0x0020,
	NETIF_MSG_RX_ERR        = 0x0040,
	NETIF_MSG_TX_ERR        = 0x0080,
	NETIF_MSG_TX_QUEUED     = 0x0100,
	NETIF_MSG_INTR          = 0x0200,
	NETIF_MSG_TX_DONE       = 0x0400,
	NETIF_MSG_RX_STATUS     = 0x0800,
	NETIF_MSG_PKTDATA       = 0x1000,
	NETIF_MSG_HW            = 0x2000,
	NETIF_MSG_WOL           = 0x4000,
};

enum netdev_tx {
	__NETDEV_TX_MIN  = INT_MIN,     /* make sure enum is signed */
	NETDEV_TX_OK     = 0x00,        /* driver took care of packet */
	NETDEV_TX_BUSY   = 0x10,        /* driver tx path was busy*/
	NETDEV_TX_LOCKED = 0x20,        /* driver tx lock was already taken */
};

struct net_device; 
struct header_ops {
	int	(*create) (struct sk_buff *skb, struct net_device *dev,
			unsigned short type, const void *daddr,
			const void *saddr, unsigned int len);
	int	(*parse)(const struct sk_buff *skb, unsigned char *haddr);
	int	(*rebuild)(struct sk_buff *skb);
};

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

typedef enum netdev_tx netdev_tx_t;
typedef u64 netdev_features_t;

struct net_device_ops {
	struct net_device_stats*	(*ndo_get_stats)(struct net_device *dev);

	int				(*ndo_open)(struct net_device *dev);
	int				(*ndo_stop)(struct net_device *dev);
	struct rtnl_link_stats64*	(*ndo_get_stats64)(struct net_device *dev, struct rtnl_link_stats64 *storage);
	netdev_tx_t			(*ndo_start_xmit) (struct sk_buff *skb, struct net_device *dev);
	void				(*ndo_tx_timeout) (struct net_device *dev);
	int				(*ndo_validate_addr)(struct net_device *dev);
	int				(*ndo_change_mtu)(struct net_device *dev, int new_mtu);
	netdev_features_t		(*ndo_fix_features)(struct net_device *dev, netdev_features_t features);
	int				(*ndo_set_features)(struct net_device *dev, netdev_features_t features);
	int				(*ndo_set_mac_address)(struct net_device *dev, void *addr);
	void				(*ndo_set_rx_mode)(struct net_device *dev);
	int				(*ndo_do_ioctl)(struct net_device *dev, struct ifreq *ifr, int cmd);
	void				(*ndo_poll_controller)(struct net_device *dev);
};

struct net_device {
	unsigned long			state;

	netdev_features_t		features;
	netdev_features_t		hw_features;
	netdev_features_t		vlan_features;

	//	PCI_Device*			dev;
	void* 				dev;

	char				name[16]; 
	int 				tx_queue; 
	int				irq; 

	struct net_device_stats		stats;
	unsigned char			dev_addr[ETH_ALEN];
	unsigned char			perm_addr[ETH_ALEN];
	const struct net_device_ops*	netdev_ops;

	const struct header_ops* 	header_ops; 

	int				watchdog_timeo;

	unsigned char			broadcast[MAX_ADDR_LEN]; 
	unsigned long			tx_queue_len; 
	unsigned int 			flags; 
	unsigned int 			priv_flags; 
	unsigned int			mtu;
	unsigned short			type; 
	unsigned short			hard_header_len; 

	unsigned char			addr_len; 

	void* 				priv; 
};

struct napi_struct {
	bool	enabled;
};

struct net_device *alloc_netdev_mqs(int sizeof_priv, const char *name,
		void (*setup)(struct net_device *),
		unsigned int txqs, unsigned int rxqs);
void free_netdev(struct net_device *dev);
void ether_setup(struct net_device *dev);
void dev_kfree_skb_any(struct sk_buff *skb);

int register_netdev(struct net_device *dev);
void unregister_netdev(struct net_device *dev);

void netif_start_queue(struct net_device *dev);
void netif_stop_queue(struct net_device *dev);
int netif_receive_skb(struct sk_buff *skb);
void netif_wake_queue(struct net_device *dev);

void *netdev_priv(const struct net_device *dev);
int __netdev_update_features(struct net_device *dev);
void netdev_features_change(struct net_device *dev); 
void netdev_update_features(struct net_device *dev);
void netdev_change_features(struct net_device *dev);
void netif_device_detach(struct net_device *dev);
void netif_carrier_on(struct net_device *dev);
void netif_carrier_off(struct net_device *dev);
void netif_start_queue(struct net_device *dev);
void netif_stop_queue(struct net_device *dev);
int netif_queue_stopped(const struct net_device *dev);
void netif_wake_queue(struct net_device *dev);
int netif_running(const struct net_device *dev);
void napi_enable(struct napi_struct* n);
void napi_disable(struct napi_struct* n);
void napi_gro_receive(struct napi_struct* n, struct sk_buff* buf);
void napi_complete(struct napi_struct *n);
void napi_schedule(struct napi_struct* n);

#endif /* __LINUX_NETDEVICE_H__ */

