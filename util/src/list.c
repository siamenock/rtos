#include <stdio.h>
#include <stdint.h>
#include <control/rpc.h>

#include "rpc.h"

static RPC* rpc;

static void help() {
	printf("Usage: list \n");
}

static int vm_list(int argc, char** argv) {
	bool callback_vm_list(uint32_t* ids, uint16_t count, void* context) {
		for(int i = 0 ; i < count; i++)
			printf("%d ", ids[i]);

		printf("\n");
		rpc_disconnect(rpc);
		return false;
	}

	rpc_vm_list(rpc, callback_vm_list, NULL);
	return 0;
}	

int main(int argc, char *argv[]) {
	char* host = DEFAULT_HOST;
	int port = DEFAULT_PORT;
	int timeout = DEFAULT_TIMEOUT;

	rpc = rpc_connect(host, port, timeout, true);
	if(rpc == NULL) {
		printf("Failed to connect\n");
		return ERROR_RPC_DISCONNECTED;
	}
	
	if(vm_list(argc, argv)) {
		printf("Failed to delete VM\n");
		rpc_disconnect(rpc);
		return ERROR_CMD_EXECUTE;
	}

	while(1) {
		if(rpc_connected(rpc))
			rpc_loop(rpc);
		else
			break;
	}
}

