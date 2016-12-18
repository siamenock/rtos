#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <util/types.h>
#include <control/rpc.h>
#include <control/vmspec.h>

#include "connect.h"

#define DEFAULT_HOST		"192.168.100.254"
#define DEFAULT_PORT		1111
#define DEFAULT_TIMEOUT		3

#define ERROR_RPC_DISCONNECTED	-10000
#define ERROR_CMD_EXECUTE	-10001
#define ERROR_MALLOC_NULL	-10002

static RPC* rpc;

static int vm_create(int argc, char* argv[]) {
	VMSpec vm;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_count = 0;
	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;
	vm.argc = 0;
	char* _args[VM_MAX_ARGC];
	vm.argv = _args;

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "core:") == 0) {
			i++;
			if(!is_uint8(argv[i])) {
				printf("core must be uint8\n");
				return i;
			}

			vm.core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "memory:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("memory must be uint32\n");
				return i;
			}

			vm.memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "storage:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("storage must be uint32\n");
				return i;
			}

			vm.storage_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "nic:") == 0) {
			i++;

			NICSpec* nic = &vm.nics[vm.nic_count];
			vm.nic_count++;

			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("mac must be uint64\n");
						return i;
					}
					nic->mac = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "dev:") == 0) {
					i++;
					nic->dev = argv[i];
				} else if(strcmp(argv[i], "ibuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("ibuf must be uint32\n");
						return i;
					}
					nic->input_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "obuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("obuf must be uint32\n");
						return i;
					}
					nic->output_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "iband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("iband must be uint64\n");
						return i;
					}
					nic->input_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "oband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("oband must be uint64\n");
						return i;
					}
					nic->output_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "hpad:") == 0) {
					i++;
					if(!is_uint8(argv[i])) {
						printf("hpad must be uint8\n");
						return i;
					}
					nic->padding_head = parse_uint8(argv[i]);
				} else if(strcmp(argv[i], "tpad:") == 0) {
					i++;
					if(!is_uint8(argv[i])) {
						printf("tpad must be uint8\n");
						return i;
					}
					nic->padding_tail = parse_uint8(argv[i]);
				} else if(strcmp(argv[i], "pool:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("pool must be uint32\n");
						return i;
					}
					nic->pool_size = parse_uint32(argv[i]);
				} else {
					i--;
					break;
				}
			}
		} else if(strcmp(argv[i], "args:") == 0) {
			i++;
			for( ; i < argc; i++)
				vm.argv[vm.argc++] = argv[i];
		}
	}

	bool callback_vm_create(uint32_t id, void* context) {
		if(id == 0)
			printf("Fail\n");
		else
			printf("%d\n", id);

		return false;
	}

	rpc_vm_create(rpc, &vm, callback_vm_create, NULL);

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
	
	if(vm_create(argc, argv)) {
		printf("Failed to create VM\n");
		rpc_disconnect(rpc);
		return ERROR_CMD_EXECUTE;
	}

	while(1)
		rpc_loop(rpc);

	return 0;
}

