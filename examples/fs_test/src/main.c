#include <stdio.h>
#include <string.h>
#include <thread.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <util/event.h>
#include <file.h>
#include <readline.h>
#include <malloc.h>
#include <timer.h>

void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

void process(NIC* ni) {
}

void destroy() {
}

void gdestroy() {
}

uint64_t before;
uint64_t after;
int read_size;
int read_fd;
void* buffer;
//
//void read_cb(void* buffer, size_t size, void* context) {
//	int total_size = *(int*)context;
//	total_size += size;
//	*(int*)context = total_size;
//	if(total_size == read_size) {
//		after = time_us();
//		printf("time : %lu\n", after - before);
//		printf("size : %dK\n", total_size / 1024);
//		printf("size : %dM\n", total_size / 1024 / 1024);
//		printf("size : %dG\n", total_size / 1024 / 1024 / 1024);
//		return;
//	} else {
////		printf("size : %dK\n", total_size / 1024);
////		printf("size : %dM\n", total_size / 1024 / 1024);
////		printf("size : %dG\n", total_size / 1024 / 1024 / 1024);
//		file_write(read_fd, buffer, read_size - total_size, read_cb, context);
//	}
//}
//
//void open_cb(int fd, void* context) {
//	if(fd >= 0) {
//		buffer = malloc(1800240);
//		if(!buffer) {
//			printf("malloc error\n"); 
//			return;
//		}
//		memset(buffer, 0x22, 1700240);
//		before = time_us();
//		read_size = 4096*256*512;
//		read_fd = fd;
//		file_write(fd, buffer, read_size, read_cb, context);
//	}
//}
void dio_cb(Dirent* dir, void* context) {
	if(*dir->name) {
		printf("name : %s\n", dir->name);
		file_readdir(*(int*)context, dir, dio_cb, context);
	}
}

void open_cb(int fd, void* context) {
	*(int*)context = fd;
	if(fd >= 0) {
		Dirent* dirent = malloc(sizeof(Dirent));
		file_readdir(fd, dirent, dio_cb, context);

	} else {
		printf("fd : %d\n", fd);
	}
}
int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		time_init();
		event_init();
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();

	/* Start of User Code Area */

	int fd;
	file_init();
	file_opendir("/", open_cb, &fd);
	if(thread_id() == 0) {
		while(1) {
			event_loop();
		}
	}
	/* End of User Code Area */

	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
