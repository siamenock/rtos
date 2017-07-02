#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_arp.h>
#include <linux/limits.h>


#include <util/event.h>
#include <net/interface.h>
#include "netob.h"
#include "driver/nicdev.h"
#include "device.h"
#include "dispatcher.h"
#include "io_mux.h"

/// @see man 7 netlink
/// @see man 7 rtnetlink
/// @see man 7 netdevice
/// @see https://tools.ietf.org/html/rfc3549
/// @see http://people.netfilter.org/pablo/netlink/netlink.pdf


static IOMultiplexer conn; ///< netob <--(netlink connection)-- linux kernel
static int interfaces;

static bool is_up(struct ifinfomsg* interface) {
	return (interface->ifi_flags & IFF_UP) == IFF_UP;
}

static bool is_loopback(struct ifinfomsg* interface) {
	return (interface->ifi_flags & IFF_LOOPBACK) == IFF_LOOPBACK;
}

static bool typeclass(char* name, char* type) {
	if(!name || !type) return false;
	char classfile[PATH_MAX] = {};
	snprintf(classfile, sizeof(classfile), "/sys/class/net/%s/%s", name, type);

	return access(classfile, F_OK) == 0;
}

static bool is_bridge(char* name) {
	return typeclass(name, "bridge");
}

static bool is_tap(char* name) {
	return typeclass(name, "tun_flags");
}

static int on_newlink_event(struct nlmsghdr *h) {
	struct ifinfomsg* info = NLMSG_DATA(h);
	int len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*info));
	int error = 0;

	char name[MAX_NIC_NAME_LEN] = {};
	int mtu = 0;
	uint64_t mac = 0;

	for(struct rtattr* attr = IFLA_RTA(info); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
		switch(attr->rta_type) {
			case IFLA_IFNAME:
				strncpy(name, RTA_DATA(attr), MAX_NIC_NAME_LEN);
				break;
			case IFLA_MTU:
				mtu = *(int *)RTA_DATA(attr);
				break;
			case IFLA_ADDRESS:
				memcpy(&mac, RTA_DATA(attr), ETH_ALEN);
				break;
			default:
				break;
		}
	}

	if(name[0] == '\0')/* || mtu == 0 || mac == 0) */
		return 1;
	if(is_loopback(info) || is_bridge(name) || is_tap(name)) // TODO Check aliased netif(eth0:0)
		return 0;


	if(is_up(info)) {
		NICDevice* nicdev = nicdev_get(name);
		if(!nicdev) {
			// Create NICDevice
			nicdev = calloc(1, sizeof(NICDevice));
			if(!nicdev) return 1;
			strncpy(nicdev->name, name,  MAX_NIC_NAME_LEN);
			nicdev->mtu = mtu;
			nicdev->mac = mac;

			error = nicdev_register(nicdev);
			if(error) {
				free(nicdev);
				return 1;
			}
			error = dispatcher_create_nicdev(nicdev);
			if(error) {
				nicdev_unregister(name);
				free(nicdev);
				return 1;
			}

			printf("netob: device '%s' is created\n", name);
		} else {
			// Update NICDevice
			nicdev->mtu = mtu;
			nicdev->mac = mac;
			printf("netob: device '%s' is updated\n", name);
		}

	} else {
		// Delete NICDevice
		NICDevice* nicdev = nicdev_unregister(name);
		if(!nicdev) return 0;
		error = dispatcher_destroy_nicdev(nicdev);
		if(error) return 1;

		printf("netob: device '%s' is deleted\n", name);
		free(nicdev);
	}

	return 0;
}

extern char* if_indextoname(unsigned int ifindex, char *ifname);
static void on_ipv4_newroute_event(struct nlmsghdr* h) {
	struct rtmsg* route = NLMSG_DATA(h);
	if(route->rtm_table != RT_TABLE_MAIN) return;

	int* ifindex = NULL;
	char name[IFNAMSIZ];
	uint32_t* address = NULL;
	uint32_t netmask = htobe32(((~0) >> (32-route->rtm_dst_len)) << (32-route->rtm_dst_len));
	uint32_t* gateway = NULL;


	struct rtattr* attr = RTM_RTA(route);
	int attrlen = RTM_PAYLOAD(h);
	for(; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen)) {
		void* value = RTA_DATA(attr);
		switch(attr->rta_type) {
			case RTA_DST:
				break;
			case RTA_GATEWAY:
				gateway = value;
				break;
			case RTA_OIF:
				ifindex = value;
				break;
			case RTA_PREFSRC:
				address = value;
				break;
			default:
				break;
		}
	}

	if(!ifindex) return;
	if(!is_tap(if_indextoname(*ifindex, name))) return;

	NICDevice* nicdev = nicdev_get(name);
	if(!nicdev) return;
	VNIC* vnic = nicdev_get_vnic_name(nicdev, name);
	if(!vnic) return;
	NIC* nic = vnic->nic;

	IPv4Interface* pnroute = interface_get_default(nic);
	if(!pnroute) {
		pnroute = interface_alloc(nic,
				address ? *address: 0, netmask,
				gateway ? *gateway : 0, true);
	} else {
		pnroute->netmask = netmask;
		if(address)
			pnroute->address = *address;
		if(gateway)
			pnroute->gateway = *gateway;
	}
}

static int observe(int fd, void* context) {
	char response[32768] = {};
	struct iovec response_vector = {response, sizeof(response)};
	struct sockaddr_nl netlinkmsg = {AF_NETLINK, 0, 0, 0};
	struct msghdr response_header = {
		.msg_name = &netlinkmsg, .msg_namelen = sizeof(netlinkmsg),
		.msg_iov = &response_vector, .msg_iovlen = 1,
		NULL, 0, 0,
	};

	ssize_t len = recvmsg(conn.fd, &response_header, 0);
	if(len < 0) return -1;

	struct nlmsghdr* msg = (struct nlmsghdr *)response;
	for(; NLMSG_OK(msg, len); msg = NLMSG_NEXT(msg, len)) {
		switch(msg->nlmsg_type) {
			case RTM_NEWLINK:
				/*Add/Remove, Up/Down, MAC, MTU, Rx/Tx Queue*/
				on_newlink_event(msg);
				break;
			case RTM_NEWADDR:
				/*IP, Run, Power Management*/
				break;
			case RTM_NEWROUTE:
				/*IP, Netmask, Gateway, Route*/
				on_ipv4_newroute_event(msg);
				break;
			case NLMSG_DONE:
				/*No more message*/
				return 0;
				break;
			default:
				break;
		}
	}

	return 0;
}

static int request_scan_all_interfaces(int socket) {
	int error = 0;
	struct iovec io = {}; // request array
	struct nlmsghdr req = {}; // request content
	{
		req.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
		req.nlmsg_type = RTM_GETLINK;
		req.nlmsg_flags = NLM_F_REQUEST | NLM_F_ATOMIC | NLM_F_DUMP;
		req.nlmsg_seq = 1;
		req.nlmsg_pid = getpid();
		io.iov_base = &req;
		io.iov_len = req.nlmsg_len;
	}
	struct msghdr rtnl_msg = {}; // reqeust object(header + content)
	struct sockaddr_nl kernel = {};
	{
		kernel.nl_family = AF_NETLINK;
		rtnl_msg.msg_iov = &io;
		rtnl_msg.msg_iovlen = 1;
		rtnl_msg.msg_name = &kernel;
		rtnl_msg.msg_namelen = sizeof(kernel);
	}

	// Send request
	error = (int)sendmsg(socket, (struct msghdr *) &rtnl_msg, 0);
	if(error <= 0) return 3;

	return error;
}

int netob_init() {
	int error = 0;

	// Create network interface query socket for metadata queries
	interfaces = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(interfaces == -1) return 1;

	// Specify netlink channel
	struct sockaddr_nl channel = {
		.nl_family = AF_NETLINK,
		.nl_pid = getpid(),
		.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE,
	};

	// Bind
	conn.fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if(conn.fd == -1) return 2;
	error = bind(conn.fd, (struct sockaddr *) &channel, sizeof(channel));
	if(error) return 3;

	// Send initial scan reqeust
	error = request_scan_all_interfaces(conn.fd);
	if(error) return 4;

	// Listen network interface status events
	conn.read_handler = observe;
	error = (int)io_mux_add(&conn, (uint64_t)&conn);
	if(error == 0) return 5;

	return 0;
}

int netob_fini() {
	io_mux_remove((uint64_t)&conn);
	if(conn.fd > 0)
		close(conn.fd);
	if(interfaces > 0)
		close(interfaces);
	return 0;
}
