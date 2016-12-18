#ifndef __CONNECT_H__
#define __CONNECT_H__

#include <control/rpc.h>

RPC* rpc_connect(char* host, int port, int timeout);
void rpc_disconnect(RPC* rpc);

#endif /* __CONNECT_H__ */
