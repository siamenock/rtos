.PHONY: all clean

all: rewrite pnkc smap console

CFLAGS := -I ../lib/core/include -Wall -Werror -std=gnu99

rewrite: src/rewrite.c
	gcc $(CFLAGS) -o $@ $<

pnkc: src/pnkc.c
	gcc $(CFLAGS) -o $@ $<

smap: src/smap.c
	gcc $(CFLAGS) -o $@ $<

src/rpc_manager.c: src/rpc_manager.x
	cd src; rpcgen -N -C -h rpc_manager.x > rpc_manager.h
	cd src; rpcgen -N -C -l rpc_manager.x > rpc_manager.c
	cd src; rpcgen -N -C -c rpc_manager.x > rpc_manager_xdr.c

src/rpc_callback_clnt.c: src/rpc_callback.x
	cd src; rpcgen -N -C -h rpc_callback.x > rpc_callback.h
	cd src; rpcgen -N -C -l rpc_callback.x > rpc_callback_clnt.c
	cd src; rpcgen -N -C -m rpc_callback.x > rpc_callback_svc.c
	cd src; rpcgen -N -C -c rpc_callback.x > rpc_callback_xdr.c

console: src/rpc_manager.c src/rpc_manager_xdr.c src/rpc_callback_clnt.c src/rpc_callback_svc.c src/rpc_callback_xdr.c src/rpc_callback_server.c src/console.c
	gcc $(CFLAGS) -Wno-unused-variable -o $@ $^ -lcurl ../lib/libcore_linux.a
	cp console ../sdk/bin/

clean:
	rm -f rewrite pnkc smap  console
	rm -f src/rpc_manager.h src/rpc_manager.c src/rpc_manager_xdr.c
	rm -f src/rpc_callback.h src/rpc_callback_clnt.c src/rpc_callback_svc.c src/rpc_callback_xdr.c
