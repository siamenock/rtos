#ifndef __LINUX_SOCKET_H__
#define __LINUX_SOCKET_H__

#include <uapi/linux/socket.h>

typedef __kernel_sa_family_t    sa_family_t;

struct sockaddr {
	sa_family_t	sa_family;	/* address family, AF_xxx       */
	char		sa_data[14];	/* 14 bytes of protocol address */
};

#endif /* __LINUX_SOCKET_H__ */

