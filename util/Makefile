.PHONY: all clean

all: rewrite pnkc smap console

rewrite: src/rewrite.c
	gcc -std=c99 -o $@ $<

pnkc: src/pnkc.c
	gcc -std=c99 -o $@ $<

smap: src/smap.c
	gcc -std=c99 -o $@ $<

src/rpc_manager.c: src/rpc_manager.x
	cd src; rpcgen -N -C -h rpc_manager.x > rpc_manager.h
	cd src; rpcgen -N -C -l rpc_manager.x > rpc_manager.c
	cd src; rpcgen -N -C -c rpc_manager.x > rpc_manager_xdr.c

src/rpc_callback_clnt.c: src/rpc_callback.x
	cd src; rpcgen -N -C -h rpc_callback.x > rpc_callback.h
	cd src; rpcgen -N -C -l rpc_callback.x > rpc_callback_clnt.c
	cd src; rpcgen -N -C -m rpc_callback.x > rpc_callback_svc.c
	cd src; rpcgen -N -C -c rpc_callback.x > rpc_callback_xdr.c

CONSOLE_OBJS = ../lib/core/obj/map.o ../lib/core/obj/list.o

console: src/rpc_manager.c src/rpc_manager_xdr.c src/rpc_callback_clnt.c src/rpc_callback_svc.c src/rpc_callback_xdr.c src/rpc_callback_server.c src/console.c src/types.c
	gcc -std=c99 -o $@ -I ../lib/core/include -lcurl $(CONSOLE_OBJS) $^
	cp console ../sdk/bin/

clean:
	rm -f rewrite pnkc smap  console
	rm -f src/rpc_manager.h src/rpc_manager.c src/rpc_manager_xdr.c
	rm -f src/rpc_callback.h src/rpc_callback_clnt.c src/rpc_callback_svc.c src/rpc_callback_xdr.c
