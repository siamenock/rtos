#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#include <net/packet.h>
#include "driver/nic.h"
#include "vnic.h"
#include "socket.h"

#define DEBUG	1

#define SOCKET_BUF_SIZE		1600

static const char ifname[IF_NAMESIZE] = "eth0";

static int request_socket(int fd, int request, struct ifreq* ifr) {
	memset(ifr, 0, sizeof(struct ifreq));
	strcpy(ifr->ifr_name, ifname);

	if(ioctl(fd, request, ifr) == -1) {
		perror("Socket request error");
		return -1;
	}

	return 0;
}

static int socket_open() {
	// Take all of ethernet packets
	int fd = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL));
	if(fd == -1) {
		perror("Socket error");
		return -1;
	}

	struct ifreq ifr;

	// Get MAC address of the interface
	if(request_socket(fd, SIOCGIFHWADDR, &ifr) == -1)
		goto error;

	uint8_t mac[ETH_ALEN];
	memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	// Get IP address of the interface
	if(request_socket(fd, SIOCGIFADDR, &ifr) == -1) {
		perror("SIOCGIFADDR");
		goto error;
	}
	struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
	char* ip = inet_ntoa(addr->sin_addr);

	printf("\tLinux RAW socket initialized\n");
	printf("\t%s\n", ifname);
	printf("\tHWaddr : %02x:%02x:%02x:%02x:%02x:%02x\n",
		       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	printf("\tInet address : %s\n", ip);

	return fd;

error:
	close(fd);
	return -1;
}

static int socket_bind(int fd) {
	// Get interface index using hard-coded name (eth0)
	struct ifreq ifr;
	if(request_socket(fd, SIOCGIFINDEX, &ifr) == -1) {
		perror("SIOCGIFINDEX");
		goto error;
	}
	int ifindex = ifr.ifr_ifindex;

	// Bind socket to eth0 interface
	struct sockaddr_ll addr_ll;
	memset(&addr_ll, 0, sizeof(struct sockaddr_ll));
	addr_ll.sll_family = AF_PACKET;
	addr_ll.sll_ifindex = ifindex;
	if(bind(fd, (struct sockaddr*)&addr_ll, sizeof(struct sockaddr_ll)) == -1) {
		perror("Bind error");
		goto error;
	}

	return 0;

error:
	close(fd);
	return -1;
}

static int socket_read(int fd, uint8_t* buffer) {
	ssize_t len = recvfrom(fd, buffer, SOCKET_BUF_SIZE, 0, NULL, NULL);

	if(len == -1)
		return -1;

#if DEBUG
	printf("\nSocket receivd: \n");
	/*
	 *for(int i = 0; i < len; i++) {
	 *        if(i % 2 == 0)
	 *                printf("%02x%02x ", buffer[i], buffer[i + 1]);
	 *        if((i + 1) % 16 == 0)
	 *                printf("\n");
	 *}
	 */
	printf("\n");
#endif
	return len;
}

static int socket_write(int fd, uint8_t* buffer, size_t size) {
	int len = sendto(fd, buffer, size, 0, NULL, 0);

	if(len == -1) {
		perror("Socket sending error");
		return -1;
	}

#if DEBUG
	printf("\nSocket written: \n");
	/*
	 *for(int i = 0; i < len; i++) {
	 *        if(i % 2 == 0)
	 *                printf("%02x%02x ", buffer[i], buffer[i + 1]);
	 *        if((i + 1) % 16 == 0)
	 *                printf("\n");
	 *}
	 */
	printf("\n");
#endif
	return len;
}

static int socket_fd;
static uint8_t socket_buffer[SOCKET_BUF_SIZE];

static int init(void* device, void* data) {
	int fd = socket_open();
	if(fd == -1)
		return false;

	if(socket_bind(fd) == -1)
		return false;

	socket_fd = fd;
	return true;
}

static void get_info(int id, NICInfo* info) {
	info->port_count = 1;

	uint8_t mac[ETH_ALEN] = "0GURUM";

	memset(&info->mac[0], 0x0, ETH_ALEN);
	for(int i = 0; i < ETH_ALEN; i++) {
		info->mac[0] |= (uint64_t)mac[i] << (ETH_ALEN - i - 1) * 8;
	}
}

static int poll(int id) {
/*
 *        int len = socket_read(socket_fd, socket_buffer);
 *        if(len <= 0)
 *                return 0;
 *
 *        if(len > 1524)
 *                printf("WARN: huge packet taken\n");
 *        // RX
 *        nic_process_input(0, socket_buffer, len, NULL, 0);
 *
 *        // TX
 *        Packet* packet = nic_process_output(0);
 *        if(packet) {
 *                printf("Packet out \n");
 *                packet_dump(packet);
 *
 *                int len = packet->end - packet->start;
 *                memcpy(socket_buffer, packet->buffer + packet->start, len);
 *                socket_write(socket_fd, socket_buffer, len);
 *        }
 */
/*
 *        uint8_t _packet[] = {
 *                0x7f, 0xff, 0xf3, 0xd5, 0x6f, 0x7d, 0x90, 0x9f,
 *                0x33, 0xa3, 0xc5, 0x70, 0x08, 0x00, 0x45, 0x00,
 *                0x00, 0x54, 0x99, 0xc0, 0x00, 0x00, 0x35, 0x01,
 *                0x10, 0xcc, 0x08, 0x08, 0x08, 0x08, 0xc0, 0xa8,
 *                0x0a, 0x65, 0x00, 0x00, 0x55, 0xce, 0x65, 0x0b,
 *                0x00, 0x0b, 0x86, 0x7c, 0xd2, 0x57, 0x00, 0x00,
 *                0x00, 0x00, 0x28, 0x74, 0x05, 0x00, 0x00, 0x00,
 *                0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
 *                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
 *                0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
 *                0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
 *                0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
 *                0x36, 0x37,
 *        };
 *
 *        // RX
 *        nic_process_input(0, _packet, sizeof(_packet), NULL, 0);
 *
 *        // TX
 *        Packet* packet = nic_process_output(0);
 *        if(packet) {
 *                printf("Packet out \n");
 *                packet_dump(packet);
 *        }
 */
	return 0;
}

static NICDriver device_driver = {
	.init = init,
	.get_info = get_info,
	.poll = poll,
	/*
	 *.destroy = destroy,
	 *.get_status = get_status,
	 *.set_status = set_status,
	 */
};

void socket_init() {
	device_register(DEVICE_TYPE_NIC, &device_driver, NULL, NULL);
}
