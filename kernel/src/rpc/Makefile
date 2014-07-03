all: rpc_manager.x rpc_callback.x
	rpcgen -N -C -h rpc_manager.x > rpc_manager.h
	rpcgen -N -C -m rpc_manager.x > rpc_manager.c
	rpcgen -N -C -c rpc_manager.x > rpc_manager_xdr.c
	rpcgen -N -C -h rpc_callback.x > rpc_callback.h
	rpcgen -N -C -m rpc_callback.x > rpc_callback.c
	rpcgen -N -C -c rpc_callback.x > rpc_callback_xdr.c

clean:
	rm -f rpc_manager.h rpc_manager.c rpc_manager_xdr.c rpc_callback.h rpc_callback.c rpc_callback_xdr.c
