#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/ether.h>
#include <util/list.h>
#include <lock.h>
#include <errno.h>
#include "tun.h"

static int get_ctl_fd(void) {
	int s_errno;
	int fd;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd >= 0)
		return fd;
	s_errno = errno;
	fd = socket(PF_PACKET, SOCK_DGRAM, 0);
	if(fd >= 0)
		return fd;
	fd = socket(PF_INET6, SOCK_DGRAM, 0);
	if(fd >= 0)
		return fd;
	errno = s_errno;
	perror("Cannot create control socket");

	return -1;
}

TunInterface* tun_create(const char* dev, int flags) {
	struct ifreq ifr;
	int fd, err;
	char *clonedev = "/dev/net/tun";
	
	if((fd = open(clonedev, O_RDWR)) < 0) {
		perror("Clone device open"); 
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = flags;   
	
	if(*dev) {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
		//perror("ioctl(TUNSETIFF)");
		close(fd);
		//exit(1);
		return NULL;
	}

	TunInterface* ti = (TunInterface*)malloc(sizeof(TunInterface));
	ti->fd = fd;
	strcpy(ti->name, ifr.ifr_name);	
	
	// default attributes 
	ti->min_buffer_size = 128;
	ti->max_buffer_size = 2048;
	ti->input_buffer = fifo_create(1048, NULL);
	ti->output_buffer = fifo_create(1048, NULL);

	int do_chflags(uint32_t flags, uint32_t mask) {
		int fd;
		int err;

		fd = get_ctl_fd();
		if(fd < 0)
			return -1;

		if((err = ioctl(fd, SIOCGIFFLAGS, &ifr)) < 0) {
			perror("ioctl(SIOCGIFFLAGS)");
			close(fd);
			return -1;
		}

		if((ifr.ifr_flags ^ flags) & mask) {
			ifr.ifr_flags &= ~mask;
			ifr.ifr_flags |= mask & flags;

			if((err = ioctl(fd, SIOCSIFFLAGS, &ifr)) < 0) {
				perror("ioctl(SIOCSIFFLAGS)");
				close(fd);
				return -1;
			}
		}

		if((err = ioctl(fd, SIOCGIFHWADDR, &ifr)) < 0) {
			perror("ioctl(SIOCGIFHWADDR)");
			close(fd);
			return -1;
		}

		uint64_t* mac = (uint64_t*)ifr.ifr_hwaddr.sa_data;
		ti->mac = endian48(*mac);

		close(fd); 

		return err;
	}
	// set tap interface up
	if(do_chflags(IFF_UP, IFF_UP) < 0) {
		perror("Interface up failed\n");
		exit(1);	
	}
	
	return ti;
}

void tun_destroy(TunInterface* ti) { 
	// destroy tap interface
	close(ti->fd);
	free(ti);
	ti = NULL;
}

//inline char* ti_input(TunInterface* ti) {
//	lock_lock(&ti->input_lock);
//
//	int	nread = read(ti->fd, ti->buffer, sizeof(ti->buffer));
//
//	// TODO : packet encapsulation & fifo pop
//
//	if(nread < 0) {
//		perror("Reading from interface");
//		close(ti->fd);
//		exit(1);
//	}
//
//	lock_unlock(&ti->input_lock);
//
//	return ti->buffer;
//}
//
//inline bool ti_output(TunInterface* ti, char* buffer) { 
//	lock_lock(&ti->output_lock);
//
//	int nwrite = write(ti->fd, ti->buffer, 42);
//
//	// TODO : fifo push
//	if(nwrite < 0) { 
//		perror("Writting to interface");
//		close(ti->fd);
//		
//		return false;
//	}		
//	
//	lock_unlock(&ti->input_lock);
//
//	return true;
//}	
