#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <util/list.h>
#include <util/cmd.h>
#include <util/types.h>
#include <errno.h>
#include <control/control.h>

#define VERSION		"0.1.0"
#define DEFAULT_HOST	"192.168.100.254"
#define DEFAULT_PORT	111
#define MAX_LINE_SIZE 2048

static bool is_continue = true;

static int cmd_exit(int argc, char** argv) {
	is_continue = false;

	return 0;
}

static int cmd_echo(int argc, char** argv) {
	int pos = 0;

	for(int i = 1; i < argc; i++) {
		pos += sprintf(cmd_result + pos, "%s", argv[i]);
		if(i + 1 < argc) {
			cmd_result[pos++] = ' ';
		}
	}

	return 0;
}

static int cmd_sleep(int argc, char** argv) {
	uint32_t time = 1;
	if(argc >= 2 && is_uint32(argv[1])) {
		time = parse_uint32(argv[1]);
	}
	sleep(time);

	return 0;
}

static void connected(bool result) {
	if(result)
		sprintf(cmd_result, "Connect success");
	else
		sprintf(cmd_result, "Connect fail");
}

static void disconnected(void) {
	sprintf(cmd_result, "Disconnected");
}

static int cmd_connect(int argc, char** argv) {
	char* host = DEFAULT_HOST;
	int port = DEFAULT_PORT;
	
	if(argc >= 2)
		host = argv[1];
	
	if(argc >= 3 && is_uint16(argv[2]))
		port = parse_uint16(argv[2]);
	
	bool ret = ctrl_connect(host, port, connected, disconnected);
	if(ret == false)
		return errno;
	else
		return 0;
}

void callback_ping(int count, clock_t delay) {
	if(delay != -1)
		printf("%d : time %ld ms\n", count, delay);
	else
		printf("%d : time out\n", count);
}

static int cmd_ping(int argc, char** argv) {
	int count = 1;
	if(argc >= 2 && is_uint64(argv[1])) {
		count = parse_uint64(argv[1]);
	}
	
	bool ret = ctrl_ping(count, callback_ping);
	if(ret == false)
		return errno;
	else
		return 0;
}

static void callback_vm_create(uint64_t vmid) {
	if(vmid == 0)
		sprintf(cmd_result, "fail");
	else
		sprintf(cmd_result, "%ld", vmid);
}

static int cmd_vm_create(int argc, char** argv) {
	VMSpec vm;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_size = 0;
	NICSpec nics[PN_MAX_NIC_COUNT];
	vm.nics = nics;
	vm.argc = 0;
	char* _args[32];
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
			
			NICSpec* nic = &vm.nics[vm.nic_size];
			vm.nic_size++;
			
			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("mac must be uint64\n");
						return i;
					}
					nic->mac = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "port:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("port must be uint32\n");
						return i;
					}
					nic->port = parse_uint32(argv[i]);
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
	
	bool ret = ctrl_vm_create(&vm, callback_vm_create);
	if(ret == false)
		return errno;
	else
		return 0;
}

static void callback_vm_delete(uint64_t vmid, bool result) {
	sprintf(cmd_result, "Machine %ld delete %s", vmid, result ? "success" : "fail");
}

static int cmd_vm_delete(int argc, char** argv) {
	if(argc < 1) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(!is_uint64(argv[1])) {
		return 1;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);

	bool ret = ctrl_vm_delete(vmid, callback_vm_delete);
	if(ret == false)
		return errno;
	else
		return 0;
}

static void callback_vm_list(uint64_t* vmids, int count) {
	int pos = 0;

	for(int i = 0 ; i < count; i++) {
		pos += sprintf(cmd_result + pos, "%ld ", vmids[i]);
	}
}

static int cmd_vm_list(int argc, char** argv) {
	bool ret = ctrl_vm_list(callback_vm_list);
	if(ret == false)
		return errno;
	else
		return 0;
}	

void progress(uint64_t vmid, uint32_t progress, uint32_t total) {
	static bool state;
	if(total == 0) {
		state = true;
		return;
	}

	if(state == true) {
		printf("\rprogress : %d total : %d => Machine %ld", progress, total, vmid);
		if(total == progress) {
			printf("\nCompleted\n");
			state = false;
		}
		return;
	}
}

static int cmd_send(int argc, char** argv) {
	if(argc < 3) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(!is_uint64(argv[1])) {
		return 1;
	}
	uint64_t vmid = parse_uint64(argv[1]);

	bool ret = ctrl_storage_upload(vmid, argv[2], progress);
	if(ret == false)
		return errno;
	else
		return 0;
}

void callback_md5(uint64_t vmid, uint32_t _md5[4]) {
	uint8_t md5[16];
	int pos = 0;
	memcpy(md5, _md5, 16);
	for(int i = 0; i < 16; i++) {
		pos += sprintf(cmd_result + pos, "%2x", md5[i]);
	}
}

static int cmd_md5(int argc, char** argv) {
	if(argc < 3) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(!is_uint64(argv[1])) {
		return 1;
	}
	
	if(!is_uint64(argv[2])) {
		return 2;
	}

	uint64_t vmid = parse_uint64(argv[1]);
	uint64_t size = parse_uint64(argv[2]);
	bool ret = ctrl_storage_md5(vmid, size, callback_md5);

	if(ret == false)
		return errno;
	else
		return 0;
}

static void callback_status_set(uint64_t vmid, int status) {
	if(status == VM_STATUS_INVALID)
		sprintf(cmd_result, "Machine %ld status set fail", vmid);
	else if(status == VM_STATUS_STOP){
		sprintf(cmd_result, "Machine %ld : stop", vmid);
	} else if(status == VM_STATUS_PAUSE) {
		sprintf(cmd_result, "Machine %ld : pause", vmid);
	} else if(status == VM_STATUS_RESUME) {
		sprintf(cmd_result, "Machine %ld : resume", vmid);
	} else if(status == VM_STATUS_START) {
		sprintf(cmd_result, "Machine %ld : start", vmid);
	}
}

static int cmd_status_set(int argc, char** argv) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(!is_uint64(argv[1])) {
		return 1;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);
	int status = VM_STATUS_STOP;

	if(strcmp(argv[0], "start") == 0) {
		status = VM_STATUS_START;
	} else if(strcmp(argv[0], "pause") == 0) {
		status = VM_STATUS_PAUSE;
	} else if(strcmp(argv[0], "resume") == 0) {
		status = VM_STATUS_RESUME;
	} else if(strcmp(argv[0], "stop") == 0) {
		status = VM_STATUS_STOP;
	}
	
	bool ret = ctrl_status_set(vmid, status, callback_status_set);
	if(ret == false)
		return errno;
	else
		return 0;
}

static void callback_status_get(uint64_t vmid, int status) {
	if(status == VM_STATUS_STOP){
		sprintf(cmd_result, "Machine %ld : stop", vmid);
	} else if(status == VM_STATUS_PAUSE) {
		sprintf(cmd_result, "Machine %ld : pause", vmid);
	} else if(status == VM_STATUS_START) {
		sprintf(cmd_result, "Machine %ld : start", vmid);
	}
}

static int cmd_status_get(int argc, char** argv) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(!is_uint64(argv[1])) {
		return 1;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);
	
	bool ret = ctrl_status_get(vmid, callback_status_get);
	if(ret == false)
		return errno;
	else
		return 0;
}

Command commands[] = {
	{
		.name = "exit",
		.desc = "Exit the console",
		.func = cmd_exit
	},
	{
		.name = "quit",
		.desc = "Exit the console",
		.func = cmd_exit
	},
	{
		.name = "help",
		.desc = "Show this message",
		.func = cmd_help
	},
	{
		.name = "echo",
		.desc = "Print variable",
		.args = "[variable: string]*",
		.func = cmd_echo
	},
	{
		.name = "sleep",
		.desc = "Sleep n seconds",
		.args = "[time: uint32]",
		.func = cmd_sleep
	},
	{
		.name = "connect",
		.desc = "Connect to the host",
		.args = "[ (host: string) [port: uint16] ]",
		.func = cmd_connect
	},
	{
		.name = "ping",
		.desc = "RPC ping to host",
		.args = "[count: uint64]",
		.func = cmd_ping
	},
	{
		.name = "create",
		.desc = "Create VM",
		.args = "vmid: uint64, core: (number: int) memory: (size: uint32) storage: (size: uint32) [nic: mac: (addr: uint64) port: (num: uint32) ibuf: (size: uint32) obuf: (size: uint32) iband: (size: uint64) oband: (size: uint64) pool: (size: uint32)]* [args: [string]+ ]",
		.func = cmd_vm_create
	},
	{
		.name = "delete",
		.desc = "Delete VM",
		.args = "result: bool, vmid: uint64",
		.func = cmd_vm_delete
	},
	{
		.name = "list",
		.desc = "List VM",
		.args = "result: uint64[]",
		.func = cmd_vm_list
	},
	{
		.name = "send",
		.desc = "Send file",
		.args = "result: bool, vmid: uint64 path: string",
		.func = cmd_send
	},
	{
		.name = "md5",
		.desc = "MD5 storage",
		.args = "result: hex16 string, vmid: uint64 size: uint64",
		.func = cmd_md5
	},
	{
		.name = "start",
		.desc = "Start VM",
		.args = "result: bool, vmid: uint64",
		.func = cmd_status_set
	},
	{
		.name = "pause",
		.desc = "Pause VM",
		.args = "result: bool, vmid: uint64",
		.func = cmd_status_set
	},
	{
		.name = "resume",
		.desc = "Resume VM",
		.args = "result: bool, vmid: uint64",
		.func = cmd_status_set
	},
	{
		.name = "stop",
		.desc = "Stop VM",
		.args = "result: bool, vmid: uint64",
		.func = cmd_status_set
	},
	{
		.name = "status",
		.desc = "Get VM's status",
		.args = "result: string(\"start\", \"pause\", or \"stop\") vmid: uint64",
		.func = cmd_status_get
	},
	{
		.name = NULL
	},
};

size_t cmd_count = sizeof(commands) / sizeof(Command);

int main(int _argc, char** _argv) {
	cmd_init();

	int execute_cmd(char* line, bool is_dump) {
		//if is_dump == true then file cmd
		//   is_dump == false then stdin cmd
		if(is_dump == true)
			printf("%s\n", line);
		
		int exit_status = cmd_exec(line);
		if(exit_status != 0) {
			if(exit_status == CMD_STATUS_WRONG_NUMBER)
				printf("wrong number of arguments\n");
			else if(exit_status == CMD_STATUS_NOT_FOUND)
				printf("wrong name of command\n");
			else if(exit_status < 0)
				printf("Error Code : %d\n", exit_status);
			else
				printf("%d'std argument type wrong\n", exit_status);
		}
		printf("> ");
		fflush(stdout);

		return exit_status;
	}
	
	List* fd_list = list_create(NULL);
	
	for(int i = 1; i < _argc; i++) {
		int fd = open(_argv[i], O_RDONLY);
		if(fd != -1) {
			list_add(fd_list, (void*)(int64_t)fd);
		}
	}

	list_add(fd_list, STDIN_FILENO); //fd of stdin

	void get_cmd_line(int fd) {
		static char line[MAX_LINE_SIZE] = {0, };
		char* head;
		int seek = 0;
		static int eod = 0; //end of data

		while((eod += read(fd, &line[eod], MAX_LINE_SIZE - eod))) {
			head = line;
			for(; seek < eod; seek++) {
				if(line[seek] == '\n') {
					line[seek] = '\0';
					int ret = execute_cmd(head, fd != STDIN_FILENO);

					if(fd != STDIN_FILENO && ret != 0 && ret != CMD_STATUS_ASYNC_CALL) {//parsing file and return error code
						printf("stop parsing file\n");
						eod = 0;
						return;
					}
					head = &line[seek] + 1;
				}
			}
			if(head == line && eod == MAX_LINE_SIZE){ //not found '\n' and head == 0
				printf("Command line is too long %d > %d\n", eod, MAX_LINE_SIZE);
				eod = 0;
				return;
			} else { //not found '\n' and seek != 0
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
		return;
	}

	int retval;
	fd_set in_put;
	fd_set temp_in;

	int fd = (int)(int64_t)list_remove_first(fd_list);
	FD_ZERO(&in_put);
	FD_SET(fd, &in_put);

	printf("> ");
	fflush(stdout);

	struct timeval time;
	
	while(is_continue) {
		temp_in = in_put;
		time.tv_sec = 0;
		time.tv_usec = 10000;

		retval = select(fd + 1, &temp_in, 0, 0, &time);//add timer

		if(retval == -1) {
			printf("selector error \n");
			return -1;
		} else if(retval == 0) {
			//select time over
		} else {
			if(FD_ISSET(fd, &temp_in) != 0) {
				get_cmd_line(fd);
				if(fd != STDIN_FILENO) { //Not stdin fd
					FD_CLR(fd, &in_put);
					close(fd);
					fd = (int)(int64_t)list_remove_first(fd_list);
					FD_SET(fd, &in_put);
				}
			}
		}
		ctrl_poll();
	}
	ctrl_disconnect();
	return 0;
}
