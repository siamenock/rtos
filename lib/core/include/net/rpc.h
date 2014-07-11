#ifndef __NET_RPC_H__
#define __NET_RPC_H__

#include <stdbool.h>
#include <stdint.h>
#include <rpc/clnt.h>
#include <net/ni.h>
#include <net/packet.h>

bool rpc_process(Packet* packet);
CLIENT* rpc_client(uint32_t ip, uint16_t port, unsigned long prognum, unsigned long versnum);
bool rpc_call_async(NetworkInterface* ni, CLIENT* client, unsigned long procnum, xdrproc_t inproc, char* in);
bool rpc_call(NetworkInterface* ni, CLIENT* client, unsigned long procnum, xdrproc_t inproc, char* in, xdrproc_t outproc, char* out, struct timeval tout, void(*succeed)(void* context, void* result), void(*failed)(void* context, int stat1, int stat2), void(*timeout)(void* context), void* context);

#endif /* __NET_RPC_H__ */
