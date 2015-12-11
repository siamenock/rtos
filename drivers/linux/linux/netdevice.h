#ifndef __LINUX_NETDEVICE_H__
#define __LINUX_NETDEVICE_H__

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/skbuff.h>
#include <linux/notifier.h>
#include <linux/list.h>
#include <linux/bitops.h>
#include <asm/barrier.h>

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
#define netdev_info(dev, fmt, args...)			printf("netdev_info: " fmt, ##args)

#define netdev_hw_addr_list_for_each(ha, l) \
	list_for_each_entry(ha, &(l)->list, list)

#define netdev_hw_addr_list_count(l) ((l)->count)
#define netdev_hw_addr_list_empty(l) (netdev_hw_addr_list_count(l) == 0)
#define netdev_mc_count(dev) netdev_hw_addr_list_count(&(dev)->mc)
#define netdev_mc_empty(dev) netdev_hw_addr_list_empty(&(dev)->mc)
#define netdev_for_each_mc_addr(ha, dev) \
	netdev_hw_addr_list_for_each(ha, &(dev)->mc)

#define netdev_uc_count(dev) netdev_hw_addr_list_count(&(dev)->uc)
#define netdev_uc_empty(dev) netdev_hw_addr_list_empty(&(dev)->uc)
#define netdev_for_each_uc_addr(ha, dev) \
	netdev_hw_addr_list_for_each(ha, &(dev)->uc)

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
	int				(*ndo_vlan_rx_add_vid)(struct net_device *dev, u16 proto, u16 vid);
	int				(*ndo_vlan_rx_kill_vid)(struct net_device *dev, u16 proto, u16 vid);
};

struct netdev_hw_addr {
        struct list_head        list;
        unsigned char           addr[MAX_ADDR_LEN];
        unsigned char           type;
#define NETDEV_HW_ADDR_T_LAN            1
#define NETDEV_HW_ADDR_T_SAN            2
#define NETDEV_HW_ADDR_T_SLAVE          3
#define NETDEV_HW_ADDR_T_UNICAST        4
#define NETDEV_HW_ADDR_T_MULTICAST      5
        bool                    global_use;
        int                     sync_cnt;
        int                     refcount;
        int                     synced;
        //struct rcu_head         rcu_head;
};

struct netdev_hw_addr_list {
	struct list_head	list;
	int			count;
};

struct netdev_queue {
/*
 * read mostly part
 */
/* Start of GurumNetworks modification
	struct net_device	*dev;
	struct Qdisc __rcu	*qdisc;
	struct Qdisc		*qdisc_sleeping;
#ifdef CONFIG_SYSFS
	struct kobject		kobj;
#endif
#if defined(CONFIG_XPS) && defined(CONFIG_NUMA)
	int			numa_node;
#endif
End of GurumNetworks modification */

/*
 * write mostly part
 */
/* Start of GurumNetworks modification
	spinlock_t		_xmit_lock ____cacheline_aligned_in_smp;
	int			xmit_lock_owner;
	
	// please use this field instead of dev->trans_start
	
	unsigned long		trans_start;

	
	//  Number of TX timeouts for this queue
	// (/sys/class/net/DEV/Q/trans_timeout)

	unsigned long		trans_timeout;
End of GurumNetworks modification */

	unsigned long		state;

/* Start of GurumNetworks modification
#ifdef CONFIG_BQL
	struct dql		dql;
#endif
	unsigned long		tx_maxrate;
End of GurumNetworks modification */
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
	unsigned long			trans_start;
	unsigned long			last_rx;

	unsigned char			broadcast[MAX_ADDR_LEN]; 
	unsigned long			tx_queue_len; 
	unsigned int 			flags; 
	unsigned int 			priv_flags; 
	unsigned int			mtu;
	unsigned short			type; 
	unsigned short			hard_header_len; 

	unsigned char			addr_len; 

	void* 				priv; 

	unsigned int			num_rx_queues;
	unsigned int			num_tx_queues;
	struct netdev_queue		*_tx;

	struct netdev_hw_addr_list	mc;
	struct netdev_hw_addr_list	uc;
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

int netif_receive_skb(struct sk_buff *skb);
void netif_wake_queue(struct net_device *dev);

void *netdev_priv(const struct net_device *dev);
int __netdev_update_features(struct net_device *dev);
void netif_tx_start_all_queues(struct net_device *dev);
void netif_tx_stop_all_queues(struct net_device *dev);
void netdev_features_change(struct net_device *dev); 
void netdev_update_features(struct net_device *dev);
void netdev_change_features(struct net_device *dev);
void netif_device_attach(struct net_device *dev);
void netif_device_detach(struct net_device *dev);
void netif_carrier_on(struct net_device *dev);
void netif_carrier_off(struct net_device *dev);
void netif_start_queue(struct net_device *dev);
void netif_stop_queue(struct net_device *dev);
int netif_set_real_num_rx_queues(struct net_device *dev, unsigned int rxq);
int netif_set_real_num_tx_queues(struct net_device *dev, unsigned int txq);
int netif_queue_stopped(const struct net_device *dev);
void netif_wake_queue(struct net_device *dev);
void netif_wake_subqueue(struct net_device *dev, u16 queue_index);
int netif_running(const struct net_device *dev);
void napi_enable(struct napi_struct* n);
void napi_disable(struct napi_struct* n);
void napi_gro_receive(struct napi_struct* n, struct sk_buff* buf);
void napi_complete(struct napi_struct *n);
void napi_schedule(struct napi_struct* n);
void napi_synchronize(const struct napi_struct *n);

enum netdev_priv_flags {
	IFF_802_1Q_VLAN			= 1<<0,
	IFF_EBRIDGE			= 1<<1,
	IFF_BONDING			= 1<<2,
	IFF_ISATAP			= 1<<3,
	IFF_WAN_HDLC			= 1<<4,
	IFF_XMIT_DST_RELEASE		= 1<<5,
	IFF_DONT_BRIDGE			= 1<<6,
	IFF_DISABLE_NETPOLL		= 1<<7,
	IFF_MACVLAN_PORT		= 1<<8,
	IFF_BRIDGE_PORT			= 1<<9,
	IFF_OVS_DATAPATH		= 1<<10,
	IFF_TX_SKB_SHARING		= 1<<11,
	IFF_UNICAST_FLT			= 1<<12,
	IFF_TEAM_PORT			= 1<<13,
	IFF_SUPP_NOFCS			= 1<<14,
	IFF_LIVE_ADDR_CHANGE		= 1<<15,
	IFF_MACVLAN			= 1<<16,
	IFF_XMIT_DST_RELEASE_PERM	= 1<<17,
	IFF_IPVLAN_MASTER		= 1<<18,
	IFF_IPVLAN_SLAVE		= 1<<19,
	IFF_VRF_MASTER			= 1<<20,
	IFF_NO_QUEUE			= 1<<21,
	IFF_OPENVSWITCH			= 1<<22,
};

#define IFF_802_1Q_VLAN			IFF_802_1Q_VLAN
#define IFF_EBRIDGE			IFF_EBRIDGE
#define IFF_BONDING			IFF_BONDING
#define IFF_ISATAP			IFF_ISATAP
#define IFF_WAN_HDLC			IFF_WAN_HDLC
#define IFF_XMIT_DST_RELEASE		IFF_XMIT_DST_RELEASE
#define IFF_DONT_BRIDGE			IFF_DONT_BRIDGE
#define IFF_DISABLE_NETPOLL		IFF_DISABLE_NETPOLL
#define IFF_MACVLAN_PORT		IFF_MACVLAN_PORT
#define IFF_BRIDGE_PORT			IFF_BRIDGE_PORT
#define IFF_OVS_DATAPATH		IFF_OVS_DATAPATH
#define IFF_TX_SKB_SHARING		IFF_TX_SKB_SHARING
#define IFF_UNICAST_FLT			IFF_UNICAST_FLT
#define IFF_TEAM_PORT			IFF_TEAM_PORT
#define IFF_SUPP_NOFCS			IFF_SUPP_NOFCS
#define IFF_LIVE_ADDR_CHANGE		IFF_LIVE_ADDR_CHANGE
#define IFF_MACVLAN			IFF_MACVLAN
#define IFF_XMIT_DST_RELEASE_PERM	IFF_XMIT_DST_RELEASE_PERM
#define IFF_IPVLAN_MASTER		IFF_IPVLAN_MASTER
#define IFF_IPVLAN_SLAVE		IFF_IPVLAN_SLAVE
#define IFF_VRF_MASTER			IFF_VRF_MASTER
#define IFF_NO_QUEUE			IFF_NO_QUEUE
#define IFF_OPENVSWITCH			IFF_OPENVSWITCH

enum netdev_state_t {
	__LINK_STATE_START,
	__LINK_STATE_PRESENT,
	__LINK_STATE_NOCARRIER,
	__LINK_STATE_LINKWATCH_PENDING,
	__LINK_STATE_DORMANT,
};

static inline bool netif_carrier_ok(const struct net_device *dev) {
	return !test_bit(__LINK_STATE_NOCARRIER, &dev->state);
}

enum netdev_queue_state_t {
	__QUEUE_STATE_DRV_XOFF,
	__QUEUE_STATE_STACK_XOFF,
	__QUEUE_STATE_FROZEN,
};

#define QUEUE_STATE_DRV_XOFF	(1 << __QUEUE_STATE_DRV_XOFF)
#define QUEUE_STATE_STACK_XOFF	(1 << __QUEUE_STATE_STACK_XOFF)
#define QUEUE_STATE_FROZEN	(1 << __QUEUE_STATE_FROZEN)
#define QUEUE_STATE_ANY_XOFF	(QUEUE_STATE_DRV_XOFF | QUEUE_STATE_STACK_XOFF)

static inline struct netdev_queue *netdev_get_tx_queue(const struct net_device *dev, unsigned int index)
{
	return &dev->_tx[index];
}

static inline void netif_tx_start_queue(struct netdev_queue *dev_queue)
{
	clear_bit(__QUEUE_STATE_DRV_XOFF, &dev_queue->state);
}

static inline void netif_tx_stop_queue(struct netdev_queue *dev_queue)
{
	set_bit(__QUEUE_STATE_DRV_XOFF, &dev_queue->state);
}

static inline void netif_stop_subqueue(struct net_device *dev, u16 queue_index)
{
	struct netdev_queue *txq = netdev_get_tx_queue(dev, queue_index);
	txq->state = 1;
	netif_tx_stop_queue(txq);
}

static inline bool netif_xmit_stopped(const struct netdev_queue *dev_queue)
{
	return dev_queue->state & QUEUE_STATE_ANY_XOFF;
}

static inline void netdev_tx_reset_queue(struct netdev_queue *dev_queue)
{
	clear_bit(__QUEUE_STATE_STACK_XOFF, &dev_queue->state);
}

static inline void netdev_tx_sent_queue(struct netdev_queue *dev_queue, unsigned int bytes)
{
	set_bit(__QUEUE_STATE_STACK_XOFF, &dev_queue->state);
}

static inline void netdev_tx_completed_queue(struct netdev_queue *dev_queue, unsigned int pkts, unsigned int bytes)
{
	if (unlikely(!bytes))
		return;

	smp_mb();

	if (test_and_clear_bit(__QUEUE_STATE_STACK_XOFF, &dev_queue->state));
}
#endif /* __LINUX_NETDEVICE_H__ */
