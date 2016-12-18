#ifndef __CONNECT_H__
#define __CONNECT_H__

#include <control/rpc.h>

#define ERROR_RPC_DISCONNECTED	-10000
#define ERROR_CMD_EXECUTE	-10001
#define ERROR_CMD_UNREGISTERED	-10002

typedef int (*RPCFunc)(int, char**); 

RPC* rpc_connect(char* host, int port, int timeout);
void rpc_disconnect(RPC* rpc);
bool rpc_is_disconnected(RPC* rpc);

void rpc_register(RPCFunc func);
int rpc_process(int argc, char** argv);

#endif /* __CONNECT_H__ */
