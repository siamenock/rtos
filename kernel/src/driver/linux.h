#ifndef __LINUX_H__
#define __LINUX_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gmalloc.h>
#include <byteswap.h>
#include <limits.h>
#include "../event.h"
#include "../pci.h"
#include "../port.h"
#include "../cpu.h"

/* Default configuration */
#define CONFIG_NET_POLL_CONTROLLER

/* Primitive types */
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef uint8_t		__u8;
typedef uint16_t	__u16;
typedef uint32_t	__u32;
typedef uint64_t	__u64;

typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef uint64_t	dma_addr_t;

/*
struct list_head {
	struct list_head *next, *prev;
};
*/

#define printk		printf
#define KERN_ERR	"KernErr: "
#define KERN_INFO	"KernInfo: "

#define HZ		1000000000UL	/*cpu_frequency*/

extern uint64_t jiffies;

#define cpu_to_le32(v)	(v)
#define cpu_to_le64(v)	(v)
#define le32_to_cpu(v)	(v)
#define le64_to_cpu(v)	(v)

#define htonl(v)	bswap_32((v))
#define htons(v)	bswap_16((v))
#define ntohl(v)	bswap_32((v))
#define ntohs(v)	bswap_16((v))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

/* Bitmap */
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT(nr)                 (1UL << (nr))
#define BITS_PER_LONG		64
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define DECLARE_BITMAP(name,bits) \
        unsigned long name[BITS_TO_LONGS(bits)]

inline int test_and_set_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;
	
	*p = old | mask;
	return (old & mask) != 0;
}

inline int test_and_clear_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;
	
	*p = old & ~mask;
	return (old & mask) != 0;
}

inline void set_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	*p |= mask;
}

inline void clear_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	*p &= ~mask;
}

inline int test_bit(int nr, const volatile unsigned long *addr) {
	return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

/* Lock */
struct u64_stats_sync {
};

inline void u64_stats_update_begin(struct u64_stats_sync* syncp) {
	// PacketNin doesn't use lock
}

inline void u64_stats_update_end(struct u64_stats_sync* syncp) {
	// PacketNin doesn't use lock
}

inline unsigned int u64_stats_fetch_begin_bh(const struct u64_stats_sync *syncp) {
	// PacketNin doesn't use lock
	return 0;
}

inline unsigned int u64_stats_fetch_retry_bh(const struct u64_stats_sync *syncp, unsigned int start) {
	// PacketNin doesn't use lock
	return 0;
}

struct mutex {
};

#define mutex_init(lock)

inline void mutex_lock(struct mutex *lock) {
}

inline int mutex_trylock(struct mutex *lock) {
	return 1;
}

inline void mutex_unlock(struct mutex *lock) {
}

/* IRQ */
#define IRQF_SHARED             0x00000080

enum irqreturn {
        IRQ_NONE                = (0 << 0),
	IRQ_HANDLED             = (1 << 0),
	IRQ_WAKE_THREAD         = (1 << 1),
};

typedef enum irqreturn irqreturn_t;
#define IRQ_RETVAL(x)   ((x) != IRQ_NONE)

typedef irqreturn_t (*irq_handler_t)(int, void *);

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev);
void free_irq(unsigned int irq, void* dev);

/* Timer */
struct timer_list {
	uint64_t	id;
	
	unsigned long expires;
	void (*function)(unsigned long);
	unsigned long data;
};

void init_timer(struct timer_list *timer);
void add_timer(struct timer_list *timer);
int mod_timer(struct timer_list *timer, unsigned long expires);
void del_timer(struct timer_list *timer);
#define del_timer_sync(timer)	del_timer(timer)

struct work_struct {
	void(*func)(struct work_struct*);
};

#define INIT_WORK(_work, _func)	(_work)->func = _func

inline bool schedule_work(struct work_struct *work) {
	event_once((void*)work->func, work);
	return true;
}

inline void cancel_work_sync(struct work_struct *work) {
}

inline void synchronize_sched() {
}

/* Annotations */
#define __iomem

/* IO */
inline void* ioremap(void* offset, unsigned long size) {
	return offset;
}

inline void iounmap(void* addr) {
}

/* PCI */
struct pci_device_id {
	__u32 vendor, device;           /* Vendor and device ID or PCI_ANY_ID*/
	__u32 subvendor, subdevice;     /* Subsystem ID's or PCI_ANY_ID */
	__u32 class, class_mask;        /* (class,subclass,prog-if) triplet */
	unsigned long driver_data;     /* Data private to the driver */
};

#define PCI_VENDOR_ID_REALTEK           0x10ec
#define PCI_VENDOR_ID_DLINK             0x1186
#define PCI_VENDOR_ID_AT                0x1259
#define PCI_VENDOR_ID_GIGABYTE          0x1458
#define PCI_VENDOR_ID_LINKSYS           0x1737

struct pci_dev {
	PCI_Device*	dev;
	void*		driver;
};

inline void pci_set_drvdata(struct pci_dev *dev, void* driver) {
	dev->driver = driver;
}

inline void* pci_get_drvdata(struct pci_dev *dev) {
	return dev->driver;
}

inline int pci_write_config_byte(const struct pci_dev *dev, int where, u8 val) {
	pci_write8(dev->dev, where, val);
	return 0;
}

inline int pci_write_config_word(const struct pci_dev *dev, int where, u16 val) {
	pci_write16(dev->dev, where, val);
	return 0;
}

inline int pci_read_config_byte(const struct pci_dev *dev, int where, u8* val) {
	*val = pci_read8(dev->dev, where);
	return 0;
}

inline int pci_read_config_word(const struct pci_dev *dev, int where, u16* val) {
	*val = pci_read16(dev->dev, where);
	return 0;
}

inline int pcie_capability_clear_word(struct pci_dev *dev, int pos, u16 clear) {
	u16 val;
	
	val = pci_read16(dev->dev, pos);
	val &= ~clear;
	pci_write16(dev->dev, pos, val);
	
	return 0;
}

inline int pcie_capability_set_word(struct pci_dev *dev, int pos, u16 set) {
	u16 val;
	
	val = pci_read16(dev->dev, pos);
	val |= set;
	pci_write16(dev->dev, pos, val);
	
	return 0;
}

inline int pcie_capability_clear_and_set_word(struct pci_dev *dev, int pos, u16 clear, u16 set) {
	u16 val;
	
	val = pci_read16(dev->dev, pos);
	val &= ~clear;
	val |= set;
	pci_write16(dev->dev, pos, val);
	
	return 0;
}

inline void pci_release_regions(struct pci_dev *pdev) {
	// TODO: Implement it
}

inline int pci_set_cacheline_size(struct pci_dev *dev) {
}

inline int pci_set_mwi(struct pci_dev *dev) {
	// TODO: Check cache line size
	uint16_t cmd = pci_read16(dev->dev, PCI_COMMAND);
	if(!(cmd & PCI_COMMAND_INVALIDATE)) {
		cmd |= PCI_COMMAND_INVALIDATE;
		pci_write16(dev->dev, PCI_COMMAND, cmd);
	}
	
	return 0;
}

inline void pci_clear_mwi(struct pci_dev *dev) {
}

inline int pci_enable_device(struct pci_dev *dev) {
	pci_enable(dev->dev);
	return 0;
}

inline void pci_disable_device(struct pci_dev *dev) {
}

inline int pci_enable_msi(struct pci_dev *dev) {
	return -1;
}

inline void pci_disable_msi(struct pci_dev *dev) {
}

inline void pci_disable_link_state(struct pci_dev *dev, int state) {
	// TODO: Implement it
}

inline dma_addr_t pci_resource_start(struct pci_dev *dev, int region) {
	uint32_t addr = pci_read32(dev->dev, PCI_BASE_ADDRESS_0 + region * 4);
	if(addr & PCI_BASE_ADDRESS_SPACE_IO)
		return addr & PCI_BASE_ADDRESS_IO_MASK;
	else
		return addr & PCI_BASE_ADDRESS_MEM_MASK;
}

inline void pci_set_master(struct pci_dev *dev) {
	// Already done when enabled
}

inline int pci_is_pcie(struct pci_dev *dev) {
	return dev->dev->capabilities[PCI_CAP_ID_EXP] != 0;
}

struct pci_driver {
	char *name;
	const struct pci_device_id *id_table;   /* must be non-NULL for probe to be called */
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);   /* New device inserted */
	void (*remove) (struct pci_dev *dev);   /* Device removed (NULL if not a hot-plug capable driver) */
	void (*shutdown)(struct pci_dev *pdev);
};

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#define DMA_MASK_NONE   0x0ULL

#define GFP_KERNEL 0

//void* dma_alloc_coherent(PCI_Device *dev, size_t size, dma_addr_t *dma_handle, int flag);
//void dma_free_coherent(PCI_Device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle);

#define DMA_FROM_DEVICE	0

inline void dma_sync_single_for_cpu(PCI_Device *dev, dma_addr_t addr, size_t size, int dir) {
}

inline void dma_sync_single_for_device(PCI_Device *dev, dma_addr_t addr, size_t size, int dir) {
}

inline int pci_set_dma_mask(struct pci_dev *dev, u64 mask) {
	return 0;
}

inline void* kmalloc_node(size_t size, int flag, int node) {
	return gmalloc(size);
}

#define dma_map_single(dev, cpu_addr, size, dir)	(cpu_addr)
#define dma_unmap_single(dev, dma_addr, size, dir)	
#define dma_mapping_error(dev, addr)	(false)

inline void* kalloc(size_t size, int flag) {
	return gmalloc(size);
}

inline void* kzalloc(size_t size, int flag) {
	void* ptr = gmalloc(size);
	bzero(ptr, size);
	
	return ptr;
}

inline void kfree(void* ptr) {
	gfree(ptr);
}

/* MII */
struct mii_if_info {
	int phy_id;
	int advertising;
	int phy_id_mask;
	int reg_num_mask;

	unsigned int full_duplex : 1;   /* is full duplex? */
	unsigned int force_media : 1;   /* is autoneg. disabled? */
	unsigned int supports_gmii : 1; /* are GMII registers supported? */

	struct net_device *dev;
	int (*mdio_read) (struct net_device *dev, int phy_id, int location);
	void (*mdio_write) (struct net_device *dev, int phy_id, int location, int val);
};

inline u8 readb(void* addr) {
	return *(u8*)addr;
}

inline u16 readw(void* addr) {
	return *(u16*)addr;
}

inline u32 readl(void* addr) {
	return *(u32*)addr;
}

inline void writeb(u8 v, void* addr) {
	*(volatile u8*)addr = v;
}

inline void writew(u16 v, void* addr) {
	*(volatile u16*)addr = v;
}

inline void writel(u32 v, void* addr) {
	*(volatile u32*)addr = v;
}

#define mmiowb()	asm volatile("" : : : "memory")

/* Module */
#define MODULE_DEVICE_TABLE(type,name)
#define MODULE_AUTHOR(name)
#define MODULE_DESCRIPTION(desc)
#define MODULE_PARM(key,value)
#define __MODULE_STRING(str)
#define MODULE_LICENSE(license)
#define SET_MODULE_OWNER(owner)
#define module_param(name,type,value)
#define module_param_named(klass,name,type,value)
#define MODULE_PARM_DESC(name,desc)
#define MODULE_VERSION(version)
#define MODULE_FIRMWARE(name)

#define module_init(fn)
#define module_exit(fn)

/* Eth tool */
#define ETH_ALEN		6
#define ETH_HLEN		14
#define ETH_ZLEN		60
#define ETH_DATA_LEN		1500

#define ETH_P_8021Q     0x8100          /* 802.1Q VLAN Extended Header  */

#define MII_BMCR                0x00    /* Basic mode control register */
#define MII_ADVERTISE           0x04    /* Advertisement control reg   */
#define MII_LPA                 0x05    /* Link partner ability reg    */
#define MII_EXPANSION           0x06    /* Expansion register          */
#define MII_CTRL1000            0x09    /* 1000BASE-T control          */

#define BMCR_RESET              0x8000  /* Reset to default state      */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_PDOWN              0x0800  /* Enable low power state      */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */

#define AUTONEG_DISABLE         0x00
#define AUTONEG_ENABLE          0x01

#define SPEED_10                10
#define SPEED_100               100
#define SPEED_1000              1000
#define SPEED_2500              2500
#define SPEED_10000             10000
#define SPEED_UNKNOWN           -1

#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */

#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */

#define ADVERTISED_10baseT_Half         (1 << 0)
#define ADVERTISED_10baseT_Full         (1 << 1)
#define ADVERTISED_100baseT_Half        (1 << 2)
#define ADVERTISED_100baseT_Full        (1 << 3)
#define ADVERTISED_1000baseT_Half       (1 << 4)
#define ADVERTISED_1000baseT_Full       (1 << 5)

#define DUPLEX_HALF             0x00
#define DUPLEX_FULL             0x01
#define DUPLEX_UNKNOWN          0xff

#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */

bool is_valid_ether_addr(const u8 *addr);

/* Socket buffer */
#define MAX_SKB_FRAGS	1

struct sockaddr {
	int		sa_family;
	unsigned char	sa_data[14];
};

struct sk_buff {
	struct net_device*	dev;
	
	unsigned int		len;
	unsigned int		tail;
	unsigned int		data_len;
	
	char*			head;
	char			data[0];
};

inline struct sk_buff *netdev_alloc_skb_ip_align(struct net_device* dev, int size) {
	struct sk_buff* skb = gmalloc(sizeof(struct sk_buff) + size);
	skb->dev = dev;
	skb->len = size;
	skb->head = skb->data;
	
	return skb;
}

inline struct sk_buff *dev_alloc_skb(unsigned int length) {
	return netdev_alloc_skb_ip_align(NULL, length);
}

inline void consume_skb(struct sk_buff *skb) {
	gfree(skb);
}

#define dev_kfree_skb(a)        consume_skb(a)

inline unsigned int skb_headlen(const struct sk_buff *skb) {
        return skb->len - skb->data_len;
}

//inline void skb_tx_timestamp(struct sk_buff *skb) {
//}

inline int skb_padto(struct sk_buff *skb, unsigned int len) {
	unsigned int size = skb->len;
	if (likely(size >= len))
		return 0;
	
	return 1;
}

inline char* skb_put(struct sk_buff *skb, int len) {
	char* tmp = skb->head + skb->tail;
	
	skb->tail += len;
	skb->len += len;
	
	return tmp;
}

inline struct sk_buff* __vlan_hwaccel_put_tag(struct sk_buff* skb, __be16 vlan_proto, u16 vlan_tci) {
	// TODO: Implement it
	return skb;
}

/* Net dev */
struct napi_struct {
	bool	enabled;
};

inline void napi_enable(struct napi_struct* n) {
	printf("napi: enable\n");
	n->enabled = true;
}

inline void napi_disable(struct napi_struct* n) {
	printf("napi: disable\n");
	n->enabled = false;
}

inline void napi_gro_receive(struct napi_struct* n, struct sk_buff* buf) {
	printf("napi: gro receive: %d\n", buf->len);
	for(int i = 0; i < buf->len; i++)
		printf("%02x", buf->data[i]);
	// TODO: Received
}

inline void napi_complete(struct napi_struct *n) {
}

inline void napi_schedule(struct napi_struct* n) {
	printf("napi: schedule\n");
}

typedef u64 netdev_features_t;

enum {
	NETIF_F_SG_BIT,                 /* Scatter/gather IO. */
	NETIF_F_IP_CSUM_BIT,            /* Can checksum TCP/UDP over IPv4. */
	__UNUSED_NETIF_F_1,
	NETIF_F_HW_CSUM_BIT,            /* Can checksum all the packets. */
	NETIF_F_IPV6_CSUM_BIT,          /* Can checksum TCP/UDP over IPV6 */
	NETIF_F_HIGHDMA_BIT,            /* Can DMA to high memory. */
	NETIF_F_FRAGLIST_BIT,           /* Scatter/gather IO. */
	NETIF_F_HW_VLAN_CTAG_TX_BIT,    /* Transmit VLAN CTAG HW acceleration */
	NETIF_F_HW_VLAN_CTAG_RX_BIT,    /* Receive VLAN CTAG HW acceleration */
	NETIF_F_HW_VLAN_CTAG_FILTER_BIT,/* Receive filtering on VLAN CTAGs */
	NETIF_F_VLAN_CHALLENGED_BIT,    /* Device cannot handle VLAN packets */
	NETIF_F_GSO_BIT,                /* Enable software GSO. */
	NETIF_F_LLTX_BIT,               /* LockLess TX - deprecated. Please */
	/* do not use LLTX in new drivers */
	NETIF_F_NETNS_LOCAL_BIT,        /* Does not change network namespaces */
	NETIF_F_GRO_BIT,                /* Generic receive offload */
	NETIF_F_LRO_BIT,                /* large receive offload */

	/**/NETIF_F_GSO_SHIFT,          /* keep the order of SKB_GSO_* bits */
	NETIF_F_TSO_BIT                 /* ... TCPv4 segmentation */
	= NETIF_F_GSO_SHIFT,
	NETIF_F_UFO_BIT,                /* ... UDPv4 fragmentation */
	NETIF_F_GSO_ROBUST_BIT,         /* ... ->SKB_GSO_DODGY */
	NETIF_F_TSO_ECN_BIT,            /* ... TCP ECN support */
	NETIF_F_TSO6_BIT,               /* ... TCPv6 segmentation */
	NETIF_F_FSO_BIT,                /* ... FCoE segmentation */
	NETIF_F_GSO_GRE_BIT,            /* ... GRE with TSO */
	NETIF_F_GSO_UDP_TUNNEL_BIT,     /* ... UDP TUNNEL with TSO */
	/**/NETIF_F_GSO_LAST =          /* last bit, see GSO_MASK */
	NETIF_F_GSO_UDP_TUNNEL_BIT,

	NETIF_F_FCOE_CRC_BIT,           /* FCoE CRC32 */
	NETIF_F_SCTP_CSUM_BIT,          /* SCTP checksum offload */
	NETIF_F_FCOE_MTU_BIT,           /* Supports max FCoE MTU, 2158 bytes*/
	NETIF_F_NTUPLE_BIT,             /* N-tuple filters supported */
	NETIF_F_RXHASH_BIT,             /* Receive hashing offload */
	NETIF_F_RXCSUM_BIT,             /* Receive checksumming offload */
	NETIF_F_NOCACHE_COPY_BIT,       /* Use no-cache copyfromuser */
	NETIF_F_LOOPBACK_BIT,           /* Enable loopback */
	NETIF_F_RXFCS_BIT,              /* Append FCS to skb pkt data */
	NETIF_F_RXALL_BIT,              /* Receive errored frames too */
	NETIF_F_HW_VLAN_STAG_TX_BIT,    /* Transmit VLAN STAG HW acceleration */
	NETIF_F_HW_VLAN_STAG_RX_BIT,    /* Receive VLAN STAG HW acceleration */
	NETIF_F_HW_VLAN_STAG_FILTER_BIT,/* Receive filtering on VLAN STAGs */

	/*
	* Add your fresh new feature above and remember to update
	* netdev_features_strings[] in net/core/ethtool.c and maybe
	* some feature mask #defines below. Please also describe it
	* in Documentation/networking/netdev-features.txt.
	*/

	/**/NETDEV_FEATURE_COUNT
};

/* copy'n'paste compression ;) */
#define __NETIF_F_BIT(bit)      ((netdev_features_t)1 << (bit))
#define __NETIF_F(name)         __NETIF_F_BIT(NETIF_F_##name##_BIT)

#define NETIF_F_FCOE_CRC        __NETIF_F(FCOE_CRC)
#define NETIF_F_FCOE_MTU        __NETIF_F(FCOE_MTU)
#define NETIF_F_FRAGLIST        __NETIF_F(FRAGLIST)
#define NETIF_F_FSO             __NETIF_F(FSO)
#define NETIF_F_GRO             __NETIF_F(GRO)
#define NETIF_F_GSO             __NETIF_F(GSO)
#define NETIF_F_GSO_ROBUST      __NETIF_F(GSO_ROBUST)
#define NETIF_F_HIGHDMA         __NETIF_F(HIGHDMA)
#define NETIF_F_HW_CSUM         __NETIF_F(HW_CSUM)
#define NETIF_F_HW_VLAN_CTAG_FILTER __NETIF_F(HW_VLAN_CTAG_FILTER)
#define NETIF_F_HW_VLAN_CTAG_RX __NETIF_F(HW_VLAN_CTAG_RX)
#define NETIF_F_HW_VLAN_CTAG_TX __NETIF_F(HW_VLAN_CTAG_TX)
#define NETIF_F_IP_CSUM         __NETIF_F(IP_CSUM)
#define NETIF_F_IPV6_CSUM       __NETIF_F(IPV6_CSUM)
#define NETIF_F_LLTX            __NETIF_F(LLTX)
#define NETIF_F_LOOPBACK        __NETIF_F(LOOPBACK)
#define NETIF_F_LRO             __NETIF_F(LRO)
#define NETIF_F_NETNS_LOCAL     __NETIF_F(NETNS_LOCAL)
#define NETIF_F_NOCACHE_COPY    __NETIF_F(NOCACHE_COPY)
#define NETIF_F_NTUPLE          __NETIF_F(NTUPLE)
#define NETIF_F_RXCSUM          __NETIF_F(RXCSUM)
#define NETIF_F_RXHASH          __NETIF_F(RXHASH)
#define NETIF_F_SCTP_CSUM       __NETIF_F(SCTP_CSUM)
#define NETIF_F_SG              __NETIF_F(SG)
#define NETIF_F_TSO6            __NETIF_F(TSO6)
#define NETIF_F_TSO_ECN         __NETIF_F(TSO_ECN)
#define NETIF_F_TSO             __NETIF_F(TSO)
#define NETIF_F_UFO             __NETIF_F(UFO)
#define NETIF_F_VLAN_CHALLENGED __NETIF_F(VLAN_CHALLENGED)
#define NETIF_F_RXFCS           __NETIF_F(RXFCS)
#define NETIF_F_RXALL           __NETIF_F(RXALL)
#define NETIF_F_GSO_GRE         __NETIF_F(GSO_GRE)
#define NETIF_F_GSO_UDP_TUNNEL  __NETIF_F(GSO_UDP_TUNNEL)
#define NETIF_F_HW_VLAN_STAG_FILTER __NETIF_F(HW_VLAN_STAG_FILTER)
#define NETIF_F_HW_VLAN_STAG_RX __NETIF_F(HW_VLAN_STAG_RX)
#define NETIF_F_HW_VLAN_STAG_TX __NETIF_F(HW_VLAN_STAG_TX)

/* Features valid for ethtool to change */
/* = all defined minus driver/device-class-related */
#define NETIF_F_NEVER_CHANGE    (NETIF_F_VLAN_CHALLENGED | NETIF_F_LLTX | NETIF_F_NETNS_LOCAL)

/* remember that ((t)1 << t_BITS) is undefined in C99 */
#define NETIF_F_ETHTOOL_BITS    ((__NETIF_F_BIT(NETDEV_FEATURE_COUNT - 1) | \
	(__NETIF_F_BIT(NETDEV_FEATURE_COUNT - 1) - 1)) & \
	~NETIF_F_NEVER_CHANGE)

/* Segmentation offload feature mask */
#define NETIF_F_GSO_MASK        (__NETIF_F_BIT(NETIF_F_GSO_LAST + 1) - \
	__NETIF_F_BIT(NETIF_F_GSO_SHIFT))

/* List of features with software fallbacks. */
#define NETIF_F_GSO_SOFTWARE    (NETIF_F_TSO | NETIF_F_TSO_ECN | \
	NETIF_F_TSO6 | NETIF_F_UFO)

#define NETIF_F_GEN_CSUM        NETIF_F_HW_CSUM
#define NETIF_F_V4_CSUM         (NETIF_F_GEN_CSUM | NETIF_F_IP_CSUM)
#define NETIF_F_V6_CSUM         (NETIF_F_GEN_CSUM | NETIF_F_IPV6_CSUM)
#define NETIF_F_ALL_CSUM        (NETIF_F_V4_CSUM | NETIF_F_V6_CSUM)

#define NETIF_F_ALL_TSO         (NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_TSO_ECN)

#define NETIF_F_ALL_FCOE        (NETIF_F_FCOE_CRC | NETIF_F_FCOE_MTU | NETIF_F_FSO)

/*
* If one device supports one of these features, then enable them
* for all in netdev_increment_features.
*/
#define NETIF_F_ONE_FOR_ALL     (NETIF_F_GSO_SOFTWARE | NETIF_F_GSO_ROBUST | \
	NETIF_F_SG | NETIF_F_HIGHDMA |         \
	NETIF_F_FRAGLIST | NETIF_F_VLAN_CHALLENGED)
/*
* If one device doesn't support one of these features, then disable it
* for all in netdev_increment_features.
*/
#define NETIF_F_ALL_FOR_ALL     (NETIF_F_NOCACHE_COPY | NETIF_F_FSO)

/* changeable features with no special hardware requirements */
#define NETIF_F_SOFT_FEATURES   (NETIF_F_GSO | NETIF_F_GRO)

/*
struct netdev_hw_addr_list {
        struct list_head        list;
        int                     count;
};
*/

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

struct rtnl_link_stats64 {
        __u64   rx_packets;             /* total packets received       */
	__u64   tx_packets;             /* total packets transmitted    */
	__u64   rx_bytes;               /* total bytes received         */
	__u64   tx_bytes;               /* total bytes transmitted      */
	__u64   rx_errors;              /* bad packets received         */
	__u64   tx_errors;              /* packet transmit problems     */
	__u64   rx_dropped;             /* no space in linux buffers    */
	__u64   tx_dropped;             /* no space available in linux  */
	__u32   rx_length_errors;
	__u64   rx_crc_errors;          /* recved pkt with crc error    */
	__u64   rx_fifo_errors;         /* recv'r fifo overrun          */
	__u64   rx_missed_errors;       /* receiver missed packet       */
};

struct net_device_ops {
	int                     (*ndo_open)(struct net_device *dev);
	int                     (*ndo_stop)(struct net_device *dev);
	struct rtnl_link_stats64* (*ndo_get_stats64)(struct net_device *dev, struct rtnl_link_stats64 *storage);
	netdev_tx_t             (*ndo_start_xmit) (struct sk_buff *skb, struct net_device *dev);
	void                    (*ndo_tx_timeout) (struct net_device *dev);
	int                     (*ndo_validate_addr)(struct net_device *dev);
	int                     (*ndo_change_mtu)(struct net_device *dev, int new_mtu);
	netdev_features_t       (*ndo_fix_features)(struct net_device *dev, netdev_features_t features);
	int                     (*ndo_set_features)(struct net_device *dev, netdev_features_t features);
	int                     (*ndo_set_mac_address)(struct net_device *dev, void *addr);
	void                    (*ndo_set_rx_mode)(struct net_device *dev);
	void                    (*ndo_poll_controller)(struct net_device *dev);
};

struct net_device {
	char				name[16];
	unsigned long			state;

	int				tx_queue;
	
	netdev_features_t       	features;
	netdev_features_t		hw_features;
	netdev_features_t		vlan_features;

	unsigned char			dev_addr[ETH_ALEN];
	unsigned char			perm_addr[ETH_ALEN];
	unsigned char			addr_len;
	
	int				mtu;
	
	const struct net_device_ops *netdev_ops;
	struct net_device_stats stats;
	unsigned long                     watchdog_timeo; /* used by dev_watchdog() */
	
	void*				priv;
	
	PCI_Device*			dev;
};

int eth_validate_addr(struct net_device *dev);

#define netdev_priv(dev)	((dev)->priv)

#define netif_err(priv, type, dev, fmt, args...)	printf("netif_err: " fmt, ##args)
#define netif_warn(priv, type, dev, fmt, args...)	printf("netif_warn: " fmt, ##args)
#define netif_info(priv, type, dev, fmt, args...)	printf("netif_info: " fmt, ##args)
#define netif_notice(priv, type, dev, fmt, args...)	printf("netif_notice: " fmt, ##args)

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

#define SET_NETDEV_DEV(_net,_dev)	((_net)->dev = *(_dev))

inline int register_netdev(struct net_device *dev) {
	printf("register netdev\n");
	// TODO: Implement it
	return 0;
}

inline void unregister_netdev(struct net_device *dev) {
	printf("unregister netdev\n");
	// TODO: Implement it
}

inline struct net_device* alloc_etherdev(int sizeof_priv) {
	struct net_device* dev = gmalloc(sizeof(struct net_device));
	bzero(dev, sizeof(struct net_device));
	memcpy(dev->name, "eth0", 5);
	dev->mtu = 1500;
	dev->addr_len = ETH_ALEN;
	dev->priv = gmalloc(sizeof_priv);
	bzero(dev->priv, sizeof_priv);
	
	return dev;
}

inline void free_netdev(struct net_device *dev) {
	gfree(dev->priv);
	gfree(dev);
}

inline netdev_features_t netdev_fix_features(struct net_device *dev, netdev_features_t features) {
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

inline int __netdev_update_features(struct net_device *dev) {
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
inline void netdev_features_change(struct net_device *dev) {
	// Do nothing
        //call_netdevice_notifiers(NETDEV_FEAT_CHANGE, dev);
}

inline void netdev_update_features(struct net_device *dev) {
        if (__netdev_update_features(dev))
                netdev_features_change(dev);
}

inline void netdev_change_features(struct net_device *dev) {
        __netdev_update_features(dev);
        netdev_features_change(dev);
}


inline void netif_device_detach(struct net_device *dev) {
	printf("netif: detatch\n");
}

inline void netif_carrier_on(struct net_device *dev) {
	printf("netif: carrier on\n");
}

inline void netif_carrier_off(struct net_device *dev) {
	printf("netif: carrier off\n");
}

inline void netif_start_queue(struct net_device *dev) {
	printf("netif: start queue\n");
	dev->tx_queue = 1;
}

inline void netif_stop_queue(struct net_device *dev) {
	dev->tx_queue = 0;
}

inline int netif_queue_stopped(const struct net_device *dev) {
	return dev->tx_queue == 0;
}

inline void netif_wake_queue(struct net_device *dev) {
	dev->tx_queue = 1;
}

inline int net_ratelimit() {
	printf("netif: ratelimit\n");
	return 1;
}

inline int netif_running(const struct net_device *dev) {
	return 1;//dev->tx_queue == 1;
}

//inline void netif_napi_add(struct net_device *dev, struct napi_struct *napi, int (*poll)(struct napi_struct *, int), int weight) {
//	printf("netif: napi add\n");
//}

//inline void netif_napi_del(struct napi_struct *napi) {
//	printf("netif: napi del\n");
//}

/* Firmware */
struct firmware {
	size_t		size;
	const u8*	data;
	void**		pages;
};

int request_firmware(const struct firmware **fw, const char *name, PCI_Device *device);
void release_firmware(const struct firmware *fw);

/* Utility */
inline void udelay(unsigned int d) {
	cpu_uwait(d);
}

inline void mdelay(unsigned int d) {
	cpu_mwait(d);
}

void msleep(unsigned int d);

#define BUG_ON(condition)	if(condition) { printf("BUG!!!\n"); while(1) asm("hlt"); }
#define BUG()			BUG_ON(true)

#define swab16	bswap_16
#define swab32	bswap_32

#define MAX_ERRNO       4095

#define ERR_PTR(error) (void*)(error)
#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)
#define IS_ERR(ptr)	IS_ERR_VALUE((unsigned long)(ptr))

inline long IS_ERR_OR_NULL(const void* ptr) {
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define __ALIGN_KERNEL_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)	__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)

inline size_t strlcpy(char* dest, const char* src, size_t size) {
	size_t ret = strlen(src);
	if(size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

#define min(x,y)	((x) < (y) ? (x) : (y))

#define wmb()		asm volatile("sfence" ::: "memory")
#define rmb()		asm volatile("lfence" ::: "memory")
#define mb()		asm volatile("mfence" ::: "memory")
#define smp_wmb()	wmb()
#define smp_rmb()	rmb()
#define smp_mb()	mb()

#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#endif /* __LINUX_H__ */
