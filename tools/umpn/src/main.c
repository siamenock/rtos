#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <tlsf.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <util/cmd.h>
#include <util/list.h>
#include <util/types.h>
#include <util/map.h>
#include <gmalloc.h>
#include <control/vmspec.h>
#include "node.h"
#include "ne.h"

#include "vm.h"

#define MAX_LINE_SIZE 	2048
#define BUFFER_SIZE 	2048	

#define ALIGN 			16 // TODO

VM* vm;

static bool is_continue = true;

static List* fd_list; 
Group* ne;
fd_set in_put, out_put;

uint8_t buffer[BUFFER_SIZE];
pid_t pid_map[16];

extern int main(int argc, char** argv);
extern void destroy();
extern void gdestroy();

uint8_t volatile* barrior_lock;
uint32_t volatile* barrior;

static int cmd_vm_status(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(vm == NULL) {
		printf("vm status invalid\n");
		return 0;
	}

	switch(vm->status) {
		case VM_STATUS_STOP:
			printf("vm status stop\n");
			break;
		case VM_STATUS_PAUSE:
			printf("vm status pause\n");
			break;
		case VM_STATUS_START:
			printf("vm status start\n");
			break;
		case VM_STATUS_RESUME:
			printf("vm status resume\n");
			break;
	}

	return 0;
}

static int cmd_vm_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	VMSpec* vmspec = malloc(sizeof(VMSpec));
	VM* _vm = NULL;
	if(!vmspec)
		return -1;

	memset(vmspec, 0, sizeof(VMSpec));

	vmspec->core_size = 1;
	vmspec->nic_count = 0;
	vmspec->nics = malloc(sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	if(!vmspec->nics)
		goto error;

	memset(vmspec->nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);

	vmspec->argv = malloc(sizeof(char*) * VM_MAX_ARGC);
	if(!vmspec->argv)
		goto error;

	memset(vmspec->argv, 0, sizeof(char*) * VM_MAX_ARGC);

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "process:") == 0) {
			i++;
			if(!is_uint8(argv[i])) {
				printf("process must be uint8\n");
				goto error;
			}
			
			vmspec->core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "nic:") == 0) {
			i++;

			NICSpec* nicspec = &vmspec->nics[vmspec->nic_count];

			nicspec->mac = 0;
			//nicspec->port = vmspec->nic_count;
			nicspec->input_buffer_size = 1024;
			nicspec->output_buffer_size = 1024;
			nicspec->pool_size = 0x400000;

			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("mac must be uint64\n");
						goto error;
					}
					nicspec->mac = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "ibuf:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("ibuf must be uint64\n"); 
						goto error; 
					} 
					nicspec->input_buffer_size = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "obuf:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("obuf must be uint64\n");
						goto error;
					}
					nicspec->output_buffer_size = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "pool:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("pool must be uint64\n");
						goto error;
					}
					nicspec->pool_size = parse_uint64(argv[i]);
				} else {
					i--;
					break;
				}
			}

			vmspec->nic_count++;
			
		} else if(strcmp(argv[i], "args:") == 0) {
			i++;
			for( ; i < argc; i++) {
				ssize_t len = strlen(argv[i]) + 1;
				vmspec->argv[vmspec->argc] = malloc(len);
				if(!vmspec->argv[vmspec->argc])
					goto error;

				strncpy(vmspec->argv[vmspec->argc++], argv[i], len);
			}
		}
	}

	_vm = malloc(sizeof(VM));
	if(_vm == NULL) {
		goto error;
	}

	memset(_vm, 0, sizeof(VM));

	_vm->core_size = vmspec->core_size;
	_vm->nic_count = vmspec->nic_count;
	_vm->nics = malloc(sizeof(NIC) * vmspec->nic_count);
	if(!_vm->nics)
		goto error;

	memset(_vm->nics, 0, sizeof(NIC) * vmspec->nic_count);

	_vm->argc = vmspec->argc;
	_vm->argv = malloc(sizeof(char*) * vmspec->argc);
	if(!_vm->argv)
		goto error;
	
	memset(_vm->argv, 0, sizeof(char*) * vmspec->argc);

	ne = ne_create(_vm->nic_count);
	if(ne == NULL)
		goto error;

	for(int i = 0; i < _vm->nic_count; i++) {
		uint64_t attrs[] = {
			NIC_MAC, ((NEPort*)ne->nodes[i])->ti->mac,
			NIC_DEV, i,
			NIC_INPUT_BUFFER_SIZE, vmspec->nics[i].input_buffer_size,
			NIC_OUTPUT_BUFFER_SIZE, vmspec->nics[i].output_buffer_size,
			NIC_INPUT_BANDWIDTH, 1000000000, //not used
			NIC_OUTPUT_BANDWIDTH, 1000000000, //not used
			NIC_POOL_SIZE, vmspec->nics[i].pool_size,
			NIC_INPUT_ACCEPT_ALL, 1,
			NIC_OUTPUT_ACCEPT_ALL, 1,
			NIC_INPUT_FUNC, 0,
			NIC_NONE
		};

		_vm->nics[i] = vnic_create(attrs);
		if(_vm->nics[i] == NULL) {
			printf("nic create error\n");
			goto error;
		}
	}
			
	for(int i = 0; i < _vm->argc; i++) {
		_vm->argv[i] = vmspec->argv[i];	
	}

	vm = _vm;

	if(vmspec->nics)
		free(vmspec->nics);
	if(vmspec->argv)
		free(vmspec->argv);
	if(vmspec)
		free(vmspec);

	return 0;

error:
	if(vmspec) {
		if(vmspec->nics)
			free(vmspec->nics);

		if(vmspec->argv) {
			for(int i = 0; i < vmspec->argc; i++) {
				if(vmspec->argv[i])
					free(vmspec->argv[i]);
			}

			free(vmspec->argv);
		}

		free(vmspec);
	}

	if(ne != NULL)
		ne_destroy(ne);
	ne = NULL;

	if(_vm) {
		if(_vm->argv)
			free(_vm->argv);

		if(_vm->nics)
			free(_vm->nics);

		free(_vm);
	}

	printf("Virtual Machine create Error\n");

	return -2;
}

static int cmd_vm_destroy(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(vm == NULL) {
		printf("Virtual Machine is not created\n");
		return -1;
	}

	if(vm->status != VM_STATUS_STOP) {
		printf("Virtual Machine is not stopped\n");
		return -2;
	}

	for(int i = 0; i < vm->argc; i++) {
		free(vm->argv[i]);
	}

	for(int i = 0; i < vm->nic_count; i++) {
		vnic_destroy(vm->nics[i]);
	}

	free(vm->argv);
	free(vm->nics);
	free(vm);
	vm = NULL;

	if(!ne_destroy(ne))
		return -3;
	ne = NULL;

	return 0;
}

static void sigterm() {
	destroy();
	extern int __thread_id;

	if(__thread_id == 0)
		gdestroy();

	printf("thread %d exit\n", __thread_id);
	exit(0);
}

static int cmd_vm_start(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(!vm) {
		return -1;
	}

	if(vm->status != VM_STATUS_STOP)
		return -2;

	for(int i = 0; i < ne->node_count; i++) {
		list_add(fd_list, (void*)(int64_t)(((NEPort*)ne->nodes[i])->ti->fd)); // tun fd
	}

	//set
	for(int i = 0; i < list_size(fd_list); i++) {
		FD_SET((int)(int64_t)list_get(fd_list, i), &in_put);
		FD_SET((int)(int64_t)list_get(fd_list, i), &out_put);
	}

	bzero((void*)barrior_lock, sizeof(uint8_t volatile));
	bzero((void*)barrior, sizeof(uint32_t volatile));

	for(int i = 0; i < vm->core_size; i++) {
		int pid = fork();

		if(pid == 0) {
			extern int __nic_count;
			extern NIC* __nics[];
			extern int __thread_id;
			extern int __thread_count;

			extern uint8_t volatile* __barrior_lock;
			extern uint32_t volatile* __barrior;

			__nic_count = vm->nic_count;
			for(int j = 0; j < vm->nic_count; j++) {
				__nics[j] = vm->nics[j]->nic;
			}
			__thread_id = i; 
			__thread_count = vm->core_size;
			__barrior_lock = barrior_lock;
			__barrior = barrior;

			signal(SIGTERM, sigterm);
			net_app_main(vm->argc, vm->argv);

			break;
		} else {
			//vm->cores[i] = pid;
			vm->cores[i] = i;
			pid_map[i] = pid;
		}
	}

	vm->status = VM_STATUS_START;
	
	return 0;
}

static int cmd_vm_stop(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(vm->status != VM_STATUS_START)
		return -1;

	for(int i = 0; i < vm->core_size; i++) {
		//kill(vm->cores[i], SIGTERM);
		//while(!(waitpid(vm->cores[i], NULL, WNOHANG) == vm->cores[i]));
		kill(pid_map[i], SIGTERM);
		while(!(waitpid(pid_map[i], NULL, WNOHANG) == pid_map[i]));
	}

	FD_ZERO(&in_put);
	FD_SET(STDIN_FILENO, &in_put);
	FD_ZERO(&out_put);

	ListIterator iter;
	list_iterator_init(&iter, fd_list);

	while(list_iterator_has_next(&iter)) {
		list_iterator_next(&iter);
		list_iterator_remove(&iter);
	}

	vm->status = VM_STATUS_STOP;

	return 0;
}

static int cmd_vm_pause(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(vm->status != VM_STATUS_START)
		return -1;

	vm->status = VM_STATUS_PAUSE;

	for(int i = 0; i < vm->core_size; i++) {
		//kill(vm->cores[i], SIGSTOP);
		kill(pid_map[i], SIGSTOP);
	}

	return 0;
}

static int cmd_vm_resume(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(vm->status != VM_STATUS_PAUSE)
		return -1;

	vm->status = VM_STATUS_START;

	for(int i = 0; i < vm->core_size; i++) {
		//kill(vm->cores[i], SIGCONT);
		kill(pid_map[i], SIGCONT);
	}

	return 0;
}

static int cmd_exit(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	is_continue = false;
	
	if(!cmd_vm_stop(0, NULL, NULL))
		return -1;

	if(!cmd_vm_destroy(0, NULL, NULL))
		return -2;

	printf("Exit\n");

	return 0;
}

Command commands[] = {
	{ 
		.name = "exit",
		.desc = "Exit the Usermode PacketNgin",
		.func = cmd_exit
	},
	{ 
		.name = "help",
		.desc = "Show this message",
		.func = cmd_help
	},
	{ 
		.name = "create",
		.desc = "Create Virtual Machine",
		.args = "process: nic: mac: args:",
		.func = cmd_vm_create
	},
	{ 
		.name = "destroy",
		.desc = "Destroy Virtual Machine",
		.func = cmd_vm_destroy
	},
	{ 
		.name = "status",
		.desc = "Status of Virtual Machine",
		.func = cmd_vm_status
	},
	{ 
		.name = "start",
		.desc = "Start Virtual Machine",
		.func = cmd_vm_start
	},
	{ 
		.name = "stop",
		.desc = "Stop Virtual Machine",
		.func = cmd_vm_stop
	},
	{ 
		.name = "pause",
		.desc = "Pause Virtual Machine",
		.func = cmd_vm_pause
	},
	{ 
		.name = "resume",
		.desc = "Resume Virtual Machine",
		.func = cmd_vm_resume
	},
	{
		.name = NULL,
		.desc = NULL,
		.args = NULL,
		.func = NULL
	}
};

static int execute_cmd(char* line, bool is_dump) {
	if(is_dump == true)
		printf("%s\n", line);
	
	int exit_status = cmd_exec(line, NULL);

	if(exit_status != 0) {
		if(exit_status == CMD_STATUS_WRONG_NUMBER) {
			printf("wrong number of arguments\n"); 
		} else if(exit_status == CMD_STATUS_NOT_FOUND) {
			printf("wrong name of command\n");
		} else if(exit_status < 0) {
			printf("error code : %d\n", exit_status);
		} else {
			printf("%d'std argument type wrong\n", exit_status); 
		}
	}
	printf("> ");
	fflush(stdout);

	return exit_status;
}

static void get_cmd_line(int fd) {
	char line[MAX_LINE_SIZE] = {0, };
	char* head;
	int seek = 0;
	int eod = 0; //end of data

	while((eod += read(fd, &line[eod], MAX_LINE_SIZE - eod))) {
		head = line;
		for(; seek < eod; seek++) {
			if(line[seek] == '\n') {
				line[seek] = '\0';
				int ret = execute_cmd(head, fd != STDIN_FILENO);
				
				if(ret == 0) {
					head = &line[seek] + 1;
				} else { 
					eod = 0;
					return;
				}
			}
		}
		if(head == line && eod == MAX_LINE_SIZE){ // not found '\n' and head == 0
			printf("Command line is too long %d > %d\n", eod, MAX_LINE_SIZE);
			eod = 0;

			return;
		} else { // not found '\n' and seek != 0
			memmove(line, head, eod - (head - line));
			eod -= head - line;
			seek = eod;
			if(fd == STDIN_FILENO) {
				return;
			} else
				continue;
		}
	}

	if(eod != 0) {
		line[eod] = '\0';
		execute_cmd(&line[0], fd != STDIN_FILENO);
	}
	eod = 0;
}

#undef main
int main(int _argc, char** _argv) { 
	cmd_init();
	extern void nic_init0();
	nic_init0();

	extern void* __gmalloc_pool;
	__gmalloc_pool = mmap(0, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(__gmalloc_pool == MAP_FAILED)
		return -1;

	init_memory_pool(0x200000, __gmalloc_pool, 1);

	barrior_lock = gmalloc(sizeof(uint8_t volatile));
	barrior = gmalloc(sizeof(uint32_t volatile));

	fd_list = list_create(NULL);

	int fd, ret;
	FD_ZERO(&in_put);
	FD_ZERO(&out_put);
	FD_SET(STDIN_FILENO, &in_put);
	
	fd_set temp_in, temp_out;

	printf("> ");
	fflush(stdout);
	while(is_continue) {
		temp_in = in_put;
		temp_out = out_put;

		fd = (int)(int64_t)list_get_last(fd_list);
		ret = select(fd + 1, &temp_in, &temp_out, 0, 0);

		if(ret == -1) {
			printf("selector error \n");

			return -1;
		} else if(ret == 0) {
			// select time over
		} else {
			if(FD_ISSET(STDIN_FILENO, &temp_in)) {
				get_cmd_line(STDIN_FILENO);
			}

			for(int i = 0; i < list_size(fd_list); i++) {
				int fd_temp = (int)(int64_t)list_get(fd_list, i);

				if(FD_ISSET(fd_temp, &temp_in)) {
					int size = read(fd_temp, buffer, BUFFER_SIZE); 
					nic_process_input(i, buffer, size, NULL, 0);
				}
			}

			for(int i = 0; i< list_size(fd_list); i++) {
				int fd_temp = (int)(int64_t)list_get(fd_list, i);

				if(FD_ISSET(fd_temp, &temp_out)) {
					Packet* packet = nic_process_output(i);

					if(packet != NULL) {
						int seek = 0;
						int len = packet->end - packet->start;
						while((seek += write(fd_temp, packet->buffer + packet->start + seek, len - seek))) {
							if(len == seek)
								break;
						}
						nic_free(packet);
					}
				}
			}
		}
	}

	return 0;
}
