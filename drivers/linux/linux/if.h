#ifndef __LINUX_IF_H__
#define __LINUX_IF_H__

#include <linux/socket.h>

#define IFNAMSIZ			16

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

#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface	*/

enum net_device_flags {
	IFF_UP				= 1<<0,  /* sysfs */
	IFF_BROADCAST			= 1<<1,  /* volatile */
	IFF_DEBUG			= 1<<2,  /* sysfs */
	IFF_LOOPBACK			= 1<<3,  /* volatile */
	IFF_POINTOPOINT			= 1<<4,  /* volatile */
	IFF_NOTRAILERS			= 1<<5,  /* sysfs */
	IFF_RUNNING			= 1<<6,  /* volatile */
	IFF_NOARP			= 1<<7,  /* sysfs */
	IFF_PROMISC			= 1<<8,  /* sysfs */
	IFF_ALLMULTI			= 1<<9,  /* sysfs */
	IFF_MASTER			= 1<<10, /* volatile */
	IFF_SLAVE			= 1<<11, /* volatile */
	IFF_MULTICAST			= 1<<12, /* sysfs */
	IFF_PORTSEL			= 1<<13, /* sysfs */
	IFF_AUTOMEDIA			= 1<<14, /* sysfs */
	IFF_DYNAMIC			= 1<<15, /* sysfs */
	IFF_LOWER_UP			= 1<<16, /* volatile */
	IFF_DORMANT			= 1<<17, /* volatile */
	IFF_ECHO			= 1<<18, /* volatile */
};

#define IFF_UP				IFF_UP
#define IFF_BROADCAST			IFF_BROADCAST
#define IFF_DEBUG			IFF_DEBUG
#define IFF_LOOPBACK			IFF_LOOPBACK
#define IFF_POINTOPOINT			IFF_POINTOPOINT
#define IFF_NOTRAILERS			IFF_NOTRAILERS
#define IFF_RUNNING			IFF_RUNNING
#define IFF_NOARP			IFF_NOARP
#define IFF_PROMISC			IFF_PROMISC
#define IFF_ALLMULTI			IFF_ALLMULTI
#define IFF_MASTER			IFF_MASTER
#define IFF_SLAVE			IFF_SLAVE
#define IFF_MULTICAST			IFF_MULTICAST
#define IFF_PORTSEL			IFF_PORTSEL
#define IFF_AUTOMEDIA			IFF_AUTOMEDIA
#define IFF_DYNAMIC			IFF_DYNAMIC
#define IFF_LOWER_UP			IFF_LOWER_UP
#define IFF_DORMANT			IFF_DORMANT
#define IFF_ECHO			IFF_ECHO

#endif /* __LINUX_IF_H__ */
