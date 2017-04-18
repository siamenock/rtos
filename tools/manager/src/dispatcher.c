#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include "dispatcher.h"

static int dispatcher_fd;

int dispatcher_init() {
	int fd = open("/dev/dispatcher", O_WRONLY);
	if(fd == -1) {
		printf("PacketNgin dispatcher module does not loaded\n");
		return -1;
	}

	//ioctl(fd, DISPATCHER_SET_MANAGER, getpid());
	printf("PacketNgin manager set to kernel dispatcher\n");

	// Register signal handler for graceful termination
	void __handler(int data) {
		int rc;
		printf("PacketNgin manager terminated...\n");
		if((rc = dispatcher_exit()) < 0)
			printf("\tDispatcher module abnormally termiated : %d\n", rc);

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

int dispatcher_create_nic(void* nic) {
	printf("Create NIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_NIC, nic);
}

int dispatcher_destroy_nic(void* nic) {
	printf("Create NIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_DESTROY_NIC, nic);
}

int dispatcher_create_vnic(void* vnic) {
	printf("Create VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

int dispatcher_destroy_vnic(void* vnic) {
	printf("Destroy VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

int dispatcher_update_vnic(void* vnic) {
	printf("Update VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

int dispatcher_get_vnic(void* vnic) {
	printf("Get VNIC to kernel dispatcher\n");
	return ioctl(dispatcher_fd, DISPATCHER_CREATE_VNIC, vnic);
}

