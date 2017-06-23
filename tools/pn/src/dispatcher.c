#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <vnic.h>
#include <net/ether.h>

#include "slowpath.h"
#include "dispatcher.h"

static int dispatcher_fd;

int dispatcher_init() {
	int fd = open("/dev/dispatcher", O_WRONLY);
	if(fd == -1) {
		printf("\tPacketNgin dispatcher module does not loaded\n");
		return -1;
	}

	//ioctl(fd, DISPATCHER_SET_MANAGER, getpid());
	printf("\tPacketNgin manager set to kernel dispatcher\n");

	// Register signal handler for graceful termination
	void __handler(int data) {
		int rc;
		printf("\tPacketNgin manager terminated...\n");
		if((rc = dispatcher_exit()) < 0)
			printf("\tDispatcher module abnormally terminated : %d\n", rc);

		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		// Gracefully kill wating for ioctl
		sleep(1);
		kill(getpid(), SIGINT);
	}

	struct sigaction act;
	act.sa_handler = __handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	dispatcher_fd = fd;
	return 0;
}

int dispatcher_exit() {
	printf("PacketNgin manager unset to kernel dispatcher\n");
	return close(dispatcher_fd);
}

int dispatcher_create_nicdev(void* nicdev) {
	printf("Create NICDev to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_NICDEV, nicdev);
}

int dispatcher_destroy_nicdev(void* nicdev) {
	printf("Create NICDev to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_DESTROY_NICDEV, nicdev);
}

int dispatcher_create_vnic(void* vnic) {
	if(slowpath_create(vnic)) return -1;

	printf("Create VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

int dispatcher_destroy_vnic(void* vnic) {
	printf("Destroy VNIC to kernel dispatcher\n");
	ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);

	slowpath_destroy(vnic);
	// FIXME: Check return value
	return 0;
}

int dispatcher_update_vnic(void* vnic) {
	printf("Update VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

int dispatcher_get_vnic(void* vnic) {
	printf("Get VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

