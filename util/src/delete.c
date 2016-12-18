#include <stdio.h>
#include <stdint.h>
#include <util/types.h>
#include <control/rpc.h>

#include "connect.h"

static RPC* rpc;

static void help() {
#define ANSI_UNDERLINED_PRE  "\033[4m"
#define ANSI_UNDERLINED_POST "\033[0m"

#define UNDERLINE(OPTION) ANSI_UNDERLINED_PRE #OPTION ANSI_UNDERLINED_POST

	printf("Usage: delete " UNDERLINE(VM ID) "\n");
}

static int vm_delete(int argc, char** argv) {
	bool callback_vm_delete(bool result, void* context) {
		if(result)
			printf("true\n");
		else
			printf("false\n");

		rpc_disconnect(rpc);
		return false;
	}

	if(argc < 2) {
		help();
		return -1;
	}

	if(!is_uint32(argv[1])) {
		help();
		return -2;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	rpc_vm_delete(rpc, vmid, callback_vm_delete, NULL);

	return 0;
}

int main(int argc, char *argv[]) {
	char* host = DEFAULT_HOST;
	int port = DEFAULT_PORT;
	int timeout = DEFAULT_TIMEOUT;

	rpc = rpc_connect(host, port, timeout);
	if(rpc == NULL) {
		printf("Failed to connect\n");
		return ERROR_RPC_DISCONNECTED;
	}
	
	int rc;
	if((rc = vm_delete(argc, argv))) {
		printf("Failed to delete VM. Error code : %d\n", rc);
		rpc_disconnect(rpc);
		return ERROR_CMD_EXECUTE;
	}

	while(1) {
		if(!rpc_is_disconnected(rpc))
			rpc_loop(rpc);
		else
			break;
	}
}

