#ifndef __RPC_H__
#define __RPC_H__

#include <control/rpc.h>

#define DEFAULT_HOST		"192.168.100.254"
#define DEFAULT_PORT		1111
#define DEFAULT_TIMEOUT		3

#define ERROR_RPC_DISCONNECTED	-10000
#define ERROR_CMD_EXECUTE	-10001
#define ERROR_MALLOC_NULL	-10002

RPC* rpc_connect(char* host, int port, int timeout, bool keep_session);
void rpc_disconnect(RPC* rpc);
bool rpc_connected(RPC* rpc);

#endif /* __RPC_H__ */
