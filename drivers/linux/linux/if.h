#ifndef __LINUX_IF_H__
#define __LINUX_IF_H__

#include <linux/socket.h>

#define IFNAMSIZ			16

/* Standard interface flags (netdevice->flags). */
#define	IFF_BROADCAST		0x2		/* broadcast address valid	*/
#define IFF_MULTICAST		0x1000		/* Supports multicast		*/

/* Private (from user) interface flags (netdevice->priv_flags). */
#define IFF_TX_SKB_SHARING	0x10000	/* The interface supports sharing
					 * skbs on transmit */

struct ifreq {
#define IFHWADDRLEN	6
	union
	{
		char	ifrn_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	} ifr_ifrn;
	
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		struct	sockaddr ifru_netmask;
		struct  sockaddr ifru_hwaddr;
		short	ifru_flags;
		int		ifru_ivalue;
		int		ifru_mtu;
		//struct  ifmap ifru_map;
		char	ifru_slave[IFNAMSIZ];	/* Just fits the size */
		char	ifru_newname[IFNAMSIZ];
		void *	ifru_data;
		//struct	if_settings ifru_settings;
	} ifr_ifru;
};


#endif /* __LINUX_IF_H__ */
