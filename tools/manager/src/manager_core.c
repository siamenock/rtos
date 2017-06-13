#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <control/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <_malloc.h>
#include <fcntl.h>
#include <util/event.h>

#include "manager.h"
#include "manager_core.h"

typedef struct {
	int	fd;
	struct	sockaddr_in caddr;
} RPCData;

static List* actives;	/* rpc */

Manager manager;

bool rpc_is_closed(RPC* rpc);
static bool manager_loop(void* context) {
	if(!list_is_empty(actives)) {
		ListIterator iter;
		list_iterator_init(&iter, actives);
		while(list_iterator_has_next(&iter)) {
			RPC* rpc = list_iterator_next(&iter);
			if(!rpc_is_closed(rpc))
				rpc_loop(rpc);
			else
				list_remove_data(actives, rpc);
		}
	}

	return true;
}

static int (*core_accept)(RPC* rpc);

int manager_core_init(int (*_accept)(RPC* rpc)) {
	printf("\tManager RPC server opened\n");

	event_idle_add(manager_loop, NULL);

	core_accept = _accept;

	return 0;
}

static int sock_read(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	int len = recv(data->fd, buf, size, MSG_DONTWAIT);
	#if DEBUG
	if(len > 0) {
		printf("Read: ");
		for(int i = 0; i < len; i++) {
			printf("%02x ", ((uint8_t*)buf)[i]);
		}
		printf("\n");
	}
	#endif /* DEBUG */

	if(len < 0) {
		if(errno == EAGAIN || errno == 0) {
			int error;
			unsigned int len = sizeof(error);
			getsockopt(data->fd, SOL_SOCKET, SO_ERROR, &error, &len);
			if(error) {
				return -1;
			}

			return 0;
		} else
			return -1;
	} else {
		return len;
	}
}

static int sock_write(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	int len = send(data->fd, buf, size, MSG_DONTWAIT);
	#if DEBUG
	if(len > 0) {
		printf("Write: ");
		for(int i = 0; i < len; i++) {
			printf("%02x ", ((uint8_t*)buf)[i]);
		}
		printf("\n");
	}
	#endif /* DEBUG */

	if(len < 0) {
		if(errno == EAGAIN || errno == 0) {
			int error;
			unsigned int len = sizeof(error);
			getsockopt(data->fd, SOL_SOCKET, SO_ERROR, &error, &len);
			if(error) {
				return -1;
			}

			return 0;
		} else
			return -1;
	} else if(len == 0) {
		return -1;
	} else {
		return len;
	}
}

static void sock_close(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	close(data->fd);
	data->fd = -1;
#if DEBUG
	printf("Connection closed : %s\n", inet_ntoa(data->caddr.sin_addr));
#endif
}

void handler(int signo) {
	// Do nothing just interrupt
}

bool rpc_is_closed(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	return data->fd < 0;

}

void rpc_close(RPC* rpc) {
	if(rpc->close)
		rpc->close(rpc);
}

void rpc_disconnect(RPC* rpc) {
	if(!rpc_is_closed(rpc))
		rpc_close(rpc);	
}

bool rpc_connected(RPC* rpc) {
	return !rpc_is_closed(rpc);
}

typedef struct {
	bool keep_session;
	uint16_t count;
	uint64_t current;
	RPC* rpc;
} HelloContext;

static bool callback_hello(void* context) {
	HelloContext* context_hello = context;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

	printf("Ping PacketNgin time = %0.3f ms\n", (float)(current - context_hello->current) / (float)1000);
	context_hello->current = current;
	context_hello->count--;
	RPC* rpc = context_hello->rpc;

	if(context_hello->count == 0) {
		if(!context_hello->keep_session) {
			rpc_disconnect(rpc);
		}
		free(context_hello);

		return false;
	}

	rpc_hello(rpc, callback_hello, context_hello);

	return true;
}

RPCData* rpc_listen(int port) {
       int fd = socket(AF_INET, SOCK_STREAM, 0);
       if(fd < 0) {
               return NULL;
       }

       int reuse = 1;
       if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
               perror("Failed to set socket option - SO_REUSEADDR\n");

       struct sockaddr_in addr;
       memset(&addr, 0x0, sizeof(struct sockaddr_in));
       addr.sin_family = AF_INET;
       addr.sin_addr.s_addr = htonl(INADDR_ANY);
       addr.sin_port = htons(port);

       if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
               return NULL;
       }

       RPCData* rpc_data = malloc(sizeof(RPCData));
       memset(rpc_data, 0x0, sizeof(RPCData));
       rpc_data->fd = fd;
	   rpc_data->caddr = addr;

       return rpc_data;
}

static RPC* rpc_accept(RPCData* rpc_data) {
       if(listen(rpc_data->fd, 5) < 0) {
               return NULL;
       }
       
       // TODO: would rather change to nonblock socket
       int rc = fcntl(rpc_data->fd, F_SETFL, fcntl(rpc_data->fd, F_GETFL, 0) | O_NONBLOCK);
       if(rc < 0)
               perror("Failed to modifiy socket to nonblock\n");
                   
       socklen_t len = sizeof(struct sockaddr_in);
       struct sockaddr_in addr;
       int fd = accept(rpc_data->fd, (struct sockaddr*)&addr, &len);
       if(fd < 0) return NULL;

       RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
       rpc->ver = 0;
       rpc->rbuf_read = 0;
       rpc->rbuf_index = 0;
       rpc->wbuf_index = 0;
       rpc->read = sock_read;
       rpc->write = sock_write;
       rpc->close = sock_close;

       RPCData* data = (RPCData*)rpc->data;
       memcpy(&data->caddr, &addr, sizeof(struct sockaddr_in));
       data->fd = fd;

       return rpc;
}

static bool manager_accept_loop(void* _rpc_data) {
	// RPC socket is nonblocking
	RPCData* rpc_data = _rpc_data;
	RPC* crpc = rpc_accept(rpc_data);
	if(!crpc)
		return true;

	core_accept(crpc);

	if(list_index_of(actives, crpc, NULL) < 0)
		list_add(actives, crpc);

	RPCData* data = (RPCData*)crpc->data;
	printf("Connection opened : %s\n", inet_ntoa(data->caddr.sin_addr));

	return true;
}

bool manager_core_server_close(ManagerCore* manager_core) {
}

ManagerCore* manager_core_server_open(uint16_t port) {
	ManagerCore* manager_core = malloc(sizeof(ManagerCore));

	RPCData* rpc_data = rpc_listen(port);
	if(!rpc_data) {
		perror("\tFailed to listen RPC server socket");
		return NULL;
	}

	if(actives == NULL)
		actives = list_create(NULL);

	event_idle_add(manager_accept_loop, (void*)rpc_data);

	return manager_core;
}
