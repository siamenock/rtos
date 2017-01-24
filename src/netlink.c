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

#include "driver/nic.h"

#define IFLIST_REPLY_BUFFER	8192
#define MAX_NET_DEVICES		8

/* PacketNgin wrapper struct for linux net_device */
typedef struct {
	int		index;
	NICStatus 	status;
	NICInfo 	info;
	NICDriver	driver;
} NetDevice;

static NetDevice* devices[MAX_NET_DEVICES];
static int netdev_count = 0;

static int init(void* device, void* data) {
	int index = *(int *)data;
	if(!devices[index]) {
		printf("Invalid net device to be initialized\n");
		return -1;
	}

	return devices[index]->index;
}

static void get_info(int id, NICInfo* info) {
	info->port_count = 1;

	memset(&info->mac[0], 0x0, ETH_ALEN);
	info->mac[0] = devices[id]->info.mac[0];

	strncpy(info->name, devices[id]->info.name, sizeof(info->name));
}

static int poll(int id) {
	return 0;
}

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

static void rtnl_link(struct nlmsghdr *h, NetDevice* device) {
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
				printf("%d: ", iface->ifi_index);
				printf("%s:\n", name);
				strncpy(device->info.name, name, sizeof(device->info.name));
				break;

			case IFLA_MTU:
				;
				int mtu = *(int *)RTA_DATA(attribute);
				printf("\tMTU %u\n", mtu);
				device->status.mtu = mtu;
				break;

			case IFLA_LINK:
				printf("\tLink %s\n", (char *) RTA_DATA(attribute));
				break;

			case IFLA_ADDRESS:
				;
				char buf[64];
				uint8_t* mac = RTA_DATA(attribute);

				memset(&device->info.mac[0], 0x0, ETH_ALEN);
				for(int i = 0; i < ETH_ALEN; i++) {
					device->info.mac[0] |=
						(uint64_t)mac[i] << (ETH_ALEN - i - 1) * 8;
				}

				printf("\tAddress %s\n",
						ll_addr_n2a(RTA_DATA(attribute),
							RTA_PAYLOAD(attribute),
							iface->ifi_type,
							buf, sizeof(buf)));
				break;

			default:
				break;
		}
	}
}

// FIXME: refactor netlink function
int netlink_init() {
	int fd;
	struct sockaddr_nl local;
	struct sockaddr_nl kernel;

	struct msghdr rtnl_msg;
	struct iovec io;

	nl_req_t req;
	char reply[IFLIST_REPLY_BUFFER];

	pid_t pid = getpid();
	int end = 0;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = pid;
	local.nl_groups = 0;

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
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
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

	while(!end) {
		int len;
		struct nlmsghdr *msg_ptr;

		struct msghdr rtnl_reply;
		struct iovec io_reply;

		memset(&io_reply, 0, sizeof(io_reply));
		memset(&rtnl_reply, 0, sizeof(rtnl_reply));

		io.iov_base = reply;
		io.iov_len = IFLIST_REPLY_BUFFER;
		rtnl_reply.msg_iov = &io;
		rtnl_reply.msg_iovlen = 1;
		rtnl_reply.msg_name = &kernel;
		rtnl_reply.msg_namelen = sizeof(kernel);

		len = recvmsg(fd, &rtnl_reply, 0);
		if(len) {
			for(msg_ptr = (struct nlmsghdr *) reply;
					NLMSG_OK(msg_ptr, len);
					msg_ptr = NLMSG_NEXT(msg_ptr, len)) {
				switch(msg_ptr->nlmsg_type) {
					case 3:		/* NLMSG_DONE */
						end++;
						break;
					case 16:	/* RTM_NEWLINK */
						;
						devices[netdev_count] = malloc(sizeof(NetDevice));
						NetDevice* device = devices[netdev_count];

						rtnl_link(msg_ptr, device);

						device->index = netdev_count;
						device->driver.init = init;
						device->driver.get_info = get_info;
						device->driver.poll = poll;

						device_register(DEVICE_TYPE_NIC, &device->driver,
							NULL, &netdev_count);
						netdev_count++;
						break;
					default:
						break;
				}
			}
		}
	}

	close(fd);

	return 0;
}


