#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <control/rpc.h>
#include "rpc.h"

void rpc_disconnect(RPC* rpc) {
	if(!rpc_is_closed(rpc))
		rpc_close(rpc);	
}

bool rpc_connected(RPC* rpc) {
	return !rpc_is_closed(rpc);
}

RPC* rpc_connect(char* host, int port, int timeout, bool keep_session) {
	typedef struct {
		bool keep_session;
		uint16_t count;
		uint64_t current;
		RPC* rpc;
	} HelloContext;

	bool callback_hello(void* context) {
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
	RPC* rpc = rpc_open(host, port, timeout);
	if(rpc == NULL)
		return NULL;

	struct timeval tv;
	gettimeofday(&tv, NULL);

	HelloContext* context_hello = malloc(sizeof(HelloContext));
	if(context_hello == NULL) {
		rpc_close(rpc);
		return NULL;
	}

	context_hello->current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
	context_hello->count = 1;
	context_hello->rpc = rpc;
	context_hello->keep_session = keep_session;

	rpc_hello(rpc, callback_hello, context_hello);

	return rpc;
}

RPCSession* rpc_session() {
	RPCSession* session = malloc(sizeof(RPCSession));
	if(!session)
		return NULL;

	session->host = getenv("MANAGER_IP");
	if(!session->host)
		return NULL;

	char* port = getenv("MANAGER_PORT");
	if(!port)
		return NULL;
	session->port = atoi(port);

	return session;
}

