#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include "dispatcher.h"

static inline int load() {
	int fd = open("/dev/dispatcher", O_WRONLY);
	if(fd == -1) {
		printf("PacketNgin dispatcher module does not loaded\n");
		return -1;
	}

	return fd;
}

int dispatcher_init() {
	int fd = load();
	if(fd < 0)
		return -1;

	ioctl(fd, DISPATCHER_SET_MANAGER, getpid());
	printf("PacketNgin manager set to kernel dispatcher\n");

	// Register signal handler for graceful termination
	void __handler(int data) {
		printf("PacketNgin manager terminated...\n");
		dispatcher_exit();
		signal(SIGINT, SIG_DFL);	
		kill(getpid(), SIGINT);
	}

	struct sigaction act;
	act.sa_handler = __handler;
	sigaction(SIGINT, &act, NULL);
	
	close(fd);
	return 0;
}

int dispatcher_exit() {
	int fd = load();
	if(fd < 0)
		return -1;

	ioctl(fd, DISPATCHER_UNSET_MANAGER, NULL);
	printf("PacketNgin manager unset to kernel dispatcher\n");

	close(fd);
	return 0;
}

int dispatcher_register_nic(void* nic) {
	int fd = load();
	if(fd < 0)
		return -1;

	ioctl(fd, DISPATCHER_REGISTER_NIC, nic);
	printf("Register manager NIC to kernel dispatcher\n");

	close(fd);
	return 0;
}

int dispatcher_unregister_nic(void) {
	int fd = load();
	if(fd < 0)
		return -1;

	ioctl(fd, DISPATCHER_UNREGISTER_NIC);
	printf("Unregister manager NIC to kernel dispatcher\n");

	close(fd);
	return 0;
}

