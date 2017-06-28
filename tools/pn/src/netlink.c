/*
   (c) Jean Lorchat @ Internet Initiative Japan - Innovation Institute
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_arp.h>
#include <linux/limits.h>
#include <util/event.h>

#include "driver/nicdev.h"
#include "device.h"
#include "dispatcher.h"

#define IFLIST_REPLY_BUFFER	8192
#define MAX_NET_DEVICES		8

/* PacketNgin wrapper struct for linux net_device */
typedef struct nl_req_s nl_req_t;

struct nl_req_s {
	struct nlmsghdr hdr;
	struct rtgenmsg gen;
};

static inline const char *ll_addr_n2a(const unsigned char *addr,
		int alen, int type, char *buf, int blen) {
	if(alen == 4 && (type == ARPHRD_TUNNEL || type == ARPHRD_SIT
	     || type == ARPHRD_IPGRE)) {
		return inet_ntop(AF_INET, addr, buf, blen);
	}

	if(alen == 16 && type == ARPHRD_TUNNEL6) {
		return inet_ntop(AF_INET6, addr, buf, blen);
	}

	snprintf(buf, blen, "%02x", addr[0]);
	for(int i = 1, l = 2; i < alen && l < blen; i++, l += 3)
		snprintf(buf + l, blen - l, ":%02x", addr[i]);

	return buf;
}

static char* nlmsghdr_getname(struct nlmsghdr* h) {
	if(!h) return NULL;
	struct ifinfomsg *iface;
	struct rtattr *attr;
	int len;

	iface = NLMSG_DATA(h);
	len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));

	for(attr= IFLA_RTA(iface); RTA_OK(attr, len); attr= RTA_NEXT(attr, len)) {
		if(attr->rta_type == IFLA_IFNAME) {
			return RTA_DATA(attr);
		}
	}
	return NULL;
}

static void rtnl_link(struct nlmsghdr *h, NICDevice* nicdev) {
	struct ifinfomsg *iface;
	struct rtattr *attribute;
	int len;

	iface = NLMSG_DATA(h);
	len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));

	for(attribute = IFLA_RTA(iface); RTA_OK(attribute, len);
			attribute = RTA_NEXT(attribute, len)) {
		switch(attribute->rta_type) {
			case IFLA_IFNAME:
				;
				char* name = RTA_DATA(attribute);
				strncpy(nicdev->name, name, MAX_NIC_NAME_LEN);
				break;

			case IFLA_MTU:
				;
				int mtu = *(int *)RTA_DATA(attribute);
				//device->status.mtu = mtu;
				break;

			case IFLA_ADDRESS:
				;
				char buf[64];
				uint8_t* mac = RTA_DATA(attribute);

				for(int i = 0; i < ETH_ALEN; i++) {
					nicdev->mac |=
						(uint64_t)mac[i] << (ETH_ALEN - i - 1) * 8;
				}

				break;

			default:
				break;
		}
	}
}

static bool interface_up_check(struct ifinfomsg *iface) {
	if((iface->ifi_flags & IFF_UP))
		return true;

	return false;
}

static bool is_loopback(struct ifinfomsg *iface) {
	return (iface->ifi_flags & IFF_LOOPBACK) == IFF_LOOPBACK;
}

static bool is_bridge(char* name) {
	if(!name) return false;
	char bridge_config[PATH_MAX] = {};
	snprintf(bridge_config, sizeof(bridge_config), "/sys/class/net/%s/bridge", name);

	return access(bridge_config, F_OK) == 0;
}

static bool is_tap(char* name) {
	if(!name) return false;
	char tap_config[PATH_MAX] = {};
	snprintf(tap_config, sizeof(tap_config), "/sys/class/net/%s/tun_flags", name);

	return access(tap_config, F_OK) == 0;
}

bool netlink_event(void* context) {
	int fd = (int)(uint64_t)context;
	int len;
	struct iovec io_reply;
	struct sockaddr_nl kernel;
	struct msghdr rtnl_reply;
	char reply[IFLIST_REPLY_BUFFER];

	memset(&io_reply, 0, sizeof(io_reply));
	memset(&kernel, 0, sizeof(kernel));
	memset(&rtnl_reply, 0, sizeof(rtnl_reply));

	io_reply.iov_base = reply;
	io_reply.iov_len = IFLIST_REPLY_BUFFER;

	kernel.nl_family = AF_NETLINK;

	rtnl_reply.msg_iov = &io_reply;
	rtnl_reply.msg_iovlen = 1;
	rtnl_reply.msg_name = &kernel;
	rtnl_reply.msg_namelen = sizeof(kernel);

	len = recvmsg(fd, &rtnl_reply, MSG_DONTWAIT);
	if(len > 0) {
		int error;
		struct nlmsghdr *msg_ptr;
		for(msg_ptr = (struct nlmsghdr *) reply; NLMSG_OK(msg_ptr, len); msg_ptr = NLMSG_NEXT(msg_ptr, len)) {
			NICDevice* nicdev;
			NICDevice* nicdev_internal;
			struct ifinfomsg *iface;
			iface = NLMSG_DATA(msg_ptr);

			char* ifname = nlmsghdr_getname(msg_ptr);
			if (is_loopback(iface) || is_bridge(ifname) || is_tap(ifname))
				continue;

			switch(msg_ptr->nlmsg_type) {
				case NLMSG_DONE: /* NLMSG_DONE */
					return true;
					break;
				case RTM_NEWLINK:
					nicdev = malloc(sizeof(NICDevice));
					memset(nicdev, 0, sizeof(NICDevice));
					rtnl_link(msg_ptr, nicdev);
					if(interface_up_check(iface)) {
						// Register resources
						if(!strncmp(nicdev->name, "v", 1)) { //Check VNIC
							free(nicdev);
							break;
						}
						error = nicdev_register(nicdev);
						if(error) {
							free(nicdev);
							break;
						}
						error = dispatcher_create_nicdev(nicdev);
						if(error) {
							free(nicdev);
							break;
						}

						printf("Register: %s\n", nicdev->name);
					} else {
						// Unregister resources
						nicdev_internal = nicdev_unregister(nicdev->name);
						if(!nicdev_internal) break;
						error = dispatcher_destroy_nicdev(nicdev);
						if(error) break;

						printf("Unregister: %s\n", nicdev->name);
						free(nicdev);
						free(nicdev_internal);
					}
					break;
				default:
					break;
			}
		}
	}

	return true;
}

// FIXME: refactor netlink function
int netlink_init() {
	int fd;
	struct sockaddr_nl local;
	struct sockaddr_nl kernel;

	struct msghdr rtnl_msg;
	struct iovec io;

	nl_req_t req;

	pid_t pid = getpid();

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = pid;
	local.nl_groups = RTNLGRP_LINK;

	if(bind(fd, (struct sockaddr *) &local, sizeof(local)) < 0) {
		perror("Failed to bind rtnetlink socket\n");
		return -1;
	}

	memset(&rtnl_msg, 0, sizeof(rtnl_msg));
	memset(&kernel, 0, sizeof(kernel));
	memset(&req, 0, sizeof(req));

	kernel.nl_family = AF_NETLINK;

	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
	req.hdr.nlmsg_type = RTM_GETLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ATOMIC | NLM_F_DUMP;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_pid = pid;
	req.gen.rtgen_family = AF_PACKET;

	io.iov_base = &req;
	io.iov_len = req.hdr.nlmsg_len;
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	rtnl_msg.msg_name = &kernel;
	rtnl_msg.msg_namelen = sizeof(kernel);

	sendmsg(fd, (struct msghdr *) &rtnl_msg, 0);
	event_idle_add(netlink_event, (void*)(uint64_t)fd);

	return 0;
}
