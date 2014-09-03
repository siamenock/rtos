#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <rpc/pmap_clnt.h>
#include <curl/curl.h>
#include <util/list.h>
#include <util/cmd.h>
#include "types.h"
#include "rpc_manager.h"
#include "rpc_callback.h"

#define VERSION		"0.1.0"
#define DEFAULT_HOST	"192.168.100.254"
#define DEFAULT_PORT	111
#define MAX_LINE_SIZE 2048

static CLIENT *client;
static CLIENT *ioclient;
static char* client_host;
static int client_port;
static int server_port;

static bool is_continue = true;

static int cmd_exit(int argc, char** argv) {
	cmd_result[0] = '\0';
	is_continue = false;
	return 0;
}

static int cmd_echo(int argc, char** argv) {
	int pos = 0;
	cmd_result[0] = '\0';
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
        cmd_result[0] = '\0';
	return 0;
}

static int cmd_connect(int argc, char** argv) {
	char* host = DEFAULT_HOST;
	int port = DEFAULT_PORT;
	
	if(argc >= 2)
		host = argv[1];
	
	if(argc >= 3 && is_uint16(argv[2]))
		port = parse_uint16(argv[2]);
	
	struct hostent* hostent = gethostbyname(host);
	if(!hostent) {
		printf("Cannot resolve the host name: %s\n", host);
		return 1;
	}
	
	struct sockaddr_in addr;
	memcpy(&addr.sin_addr, hostent->h_addr_list[0], hostent->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	struct timeval time;
	time.tv_sec = 2;
	time.tv_usec = 0;
	
	int sock = RPC_ANYSOCK;
	client = clntudp_create(&addr, MANAGER, MANAGER_APPLE, time, &sock);
	if(!client) {
		printf("Cannot make a connection to the host: %s:%d\n", host, port);
		return 2;
	}
	
	void* ret = manager_null_1(client);
	if(ret == NULL) {
		printf("Cannot connect to the host: %s:%d\n", host, port);
		client = NULL;
		return 3;
	}
	
	ioclient = clntudp_create(&addr, CALLBACK, CALLBACK_APPLE, time, &sock);
	if(!ioclient) {
		printf("Cannot make a connection to the host: %s:%d\n", host, port);
		return 2;
	}
	
	ret = callback_null_1(ioclient);
	if(ret == NULL) {
		printf("Cannot connect to the host: %s:%d\n", host, port);
		client = NULL;
		return 3;
	}
	
	RPC_Address cbaddr;
	cbaddr.ip = 0;
	cbaddr.port = server_port;
	
	bool_t* ret2 = callback_add_1(cbaddr, client);
	if(ret2 == NULL || !*ret2) {
		printf("Cannot register callback service to the host: %s:%d\n", host, port);
		return 4;
	}
	
	if(client_host) {
		free(client_host);
	}
	int len = strlen(host) + 1;
	client_host = malloc(len);
	memcpy(client_host, host, len);
	
	client_port = port;

        cmd_result[0] = '\0';
	return 0;
}

static int cmd_ping(int argc, char** argv) {
	int count = 1;
	if(argc >= 2 && is_uint64(argv[1])) {
		count = parse_uint64(argv[1]);
	}
	
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}	
	
	clock_t total = clock();
	for(int i = 0; i < count; i++) {
		clock_t time = clock();
		void* ret = manager_null_1(client);
		if(ret != NULL) {
			printf("time=%ld ms\n", clock() - time);
		} else {
			printf("timeout\n");
		}
	}
	
	printf("total: %ld ms\n", clock() - total);

	cmd_result[0] = '\0';
	return 0;
}

static int cmd_vm_create(int argc, char** argv) {
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}
	
	RPC_VM vm;
	vm.core_num = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nics.nics_len = 0;
	RPC_NIC nics[RPC_MAX_NIC_COUNT];
	vm.nics.nics_val = nics;
	vm.args.args_len = 0;
	char _args[RPC_MAX_ARGS_SIZE];
	vm.args.args_val = _args;
	
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "core:") == 0) {
			i++;
			if(!is_uint8(argv[i])) {
				printf("core must be uint8\n");
				return -1;
			}
			
			vm.core_num = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "memory:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("memory must be uint32\n");
				return -1;
			}
			
			vm.memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "storage:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("storage must be uint32\n");
				return -1;
			}
			
			vm.storage_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "nic:") == 0) {
			i++;
			
			RPC_NIC* nic = &vm.nics.nics_val[vm.nics.nics_len];
			vm.nics.nics_len++;
			
			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("mac must be uint64\n");
						return -1;
					}
					nic->mac = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "ibuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("ibuf must be uint32\n");
						return -1;
					}
					nic->input_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "obuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("obuf must be uint32\n");
						return -1;
					}
					nic->output_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "iband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("iband must be uint64\n");
						return -1;
					}
					nic->input_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "oband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("oband must be uint64\n");
						return -1;
					}
					nic->output_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "pool:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("pool must be uint32\n");
						return -1;
					}
					nic->pool_size = parse_uint32(argv[i]);
				} else {
					i--;
					break;
				}
			}
		} else if(strcmp(argv[i], "args:") == 0) {
			i++;
			
			void cpy(char* str, int len, char end) {
				memcpy(vm.args.args_val + vm.args.args_len, str, len);
				vm.args.args_len += len + 1;
				vm.args.args_val[vm.args.args_len - 1] = end;
			}
			
			for( ; i < argc; i++) {
				char* str = argv[i];
				
				if(str[0] == '"' || str[0] == '\'') {
					char dil = str[0];
					str++;
					
					do {
						int len = strlen(str);
						if(str[len - 1] != dil) {
							cpy(str, len, ' ');
						} else {
							cpy(str, len - 1, '\0');
							break;
						}
						
						str = argv[++i];
					} while(i < argc);
				} else {
					int len = strlen(str);
					cpy(str, len, '\0');
				}
			}
		}
	}
	
	u_quad_t* vmid = vm_create_1(vm, client);
	if(vmid == NULL) {
		printf("Timeout\n");
		return 2;
	}
	
	sprintf(cmd_result, "%ld", *vmid);
	
	return 0;
}

static int cmd_vm_delete(int argc, char** argv) {
	if(argc < 1) {
		return -100;
	}
	
	if(!is_uint64(argv[1])) {
		return -1;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);
	
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}
	
	bool_t* ret = vm_delete_1(vmid, client);
	if(ret == NULL) {
		printf("Timeout\n");
		return 2;
	}
	
	sprintf(cmd_result, "%s", *ret ? "true" : "false");
	
	return 0;
}

static int cmd_vm_list(int argc, char** argv) {
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}
	
	RPC_VMList* ret = vm_list_1(client);
	if(ret == NULL) {
		printf("Timeout\n");
		return 2;
	}
	
	char* p = cmd_result;
	cmd_result[0] = '\0';
	for(int i = 0; i < ret->RPC_VMList_len; i++) {
		p += sprintf(p, "%lu", ret->RPC_VMList_val[i]);
		if(i + 1 < ret->RPC_VMList_len) {
			*p++ = ' ';
		} else {
			*p++ = '\0';
		}
	}
	
	return 0;
}	

static int cmd_send(int argc, char** argv) {
	if(argc < 3) {
		return -100;
	}
	
	if(!is_uint64(argv[1])) {
		return -1;
	}
	
	CURL* curl = curl_easy_init();
	if(!curl) {
		printf("Cannot make CURL\n");
		return 1;
	}
	
	FILE* file = fopen(argv[2], "r");
	if(file == NULL) {
		printf("Cannot open file: %s\n", argv[2]);
		return 1;
	}
	
	struct stat stat;
	if(fstat(fileno(file), &stat) < 0) {
		printf("Cannot get state of file: %s\n", argv[2]);
		return 2;
	}
	
	char url[1024];
	snprintf(url, 1024, "tftp://%s/%s\n", client_host, argv[1]);
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, file);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)stat.st_size);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	
	CURLcode res;
	if((res = curl_easy_perform(curl)) == CURLE_OK) {
		sprintf(cmd_result, "true");
		
		double speed, total;
		curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed);
		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
		
		printf("%.3f bytes/sec total %.3f seconds\n", speed, total);
	} else {
		printf("Send failed: %s\n", curl_easy_strerror(res));
		sprintf(cmd_result, "false");
	}
	
	curl_easy_cleanup(curl);
	fclose(file);
	
	return 0;
}

static int cmd_md5(int argc, char** argv) {
	if(argc < 3) {
		return -100;
	}
	
	if(!is_uint64(argv[1])) {
		return -1;
	}
	
	if(!is_uint64(argv[2])) {
		return -2;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);
	uint64_t size = parse_uint64(argv[2]);
	
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}
	
	RPC_Digest* ret = storage_digest_1(vmid, RPC_MD5, size, client);
	if(ret == NULL) {
		return -3;
	}
	
	if(ret->RPC_Digest_len == 0) {
		sprintf(cmd_result, "(nil)");
	} else {
		char* p = (char*)cmd_result;
		for(int i = 0; i < ret->RPC_Digest_len; i++, p += 2) {
			sprintf(p, "%02x", ((uint8_t*)ret->RPC_Digest_val)[i]);
		}
		*p = '\0';
	}
	
	return 0;
}

static int cmd_status_set(int argc, char** argv) {
	if(argc < 2) {
		return -100;
	}
	
	if(!is_uint64(argv[1])) {
		return -1;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);
	RPC_VMStatus status = RPC_STOP;
	if(strcmp(argv[0], "start") == 0) {
		status = RPC_START;
	} else if(strcmp(argv[0], "pause") == 0) {
		status = RPC_PAUSE;
	} else if(strcmp(argv[0], "resume") == 0) {
		status = RPC_RESUME;
	} else if(strcmp(argv[0], "stop") == 0) {
		status = RPC_STOP;
	}
	
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}
	
	bool_t* ret = status_set_1(vmid, status, client);
	if(ret == NULL) {
		printf("Timeout\n");
		return 2;
	}
	
	sprintf(cmd_result, "%s", *ret ? "true" : "false");
	
	return 0;
}

static int cmd_status_get(int argc, char** argv) {
	if(argc < 2) {
		return -100;
	}
	
	if(!is_uint64(argv[1])) {
		return -1;
	}
	
	uint64_t vmid = parse_uint64(argv[1]);
	
	if(!client) {
		printf("Disconnected\n");
		return 1;
	}
	
	RPC_VMStatus* status = status_get_1(vmid, client);
	if(status == NULL) {
		printf("Timeout\n");
		return 2;
	}
	
	char* ret;
	switch(*status) {
		case RPC_START:
			ret = "start";
			break;
		case RPC_PAUSE:
			ret = "pause";
			break;
		case RPC_STOP:
			ret = "stop";
			break;
		case RPC_NONE:
		default:
			ret = "none";
	}
	
	sprintf(cmd_result, "%s", ret);
	
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
		.args = "vmid: uint64, core: (number: int) memory: (size: uint32) storage: (size: uint32) [nic: mac: (addr: uint64) ibuf: (size: uint32) obuf: (size: uint32) iband: (size: uint64) oband: (size: uint64) pool: (size: uint32)]* [args: [string]+ ]",
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
		.name = NULL,
		.desc = NULL,
		.args = NULL,
		.func = NULL
	},
};

size_t cmd_count = sizeof(commands) / sizeof(Command);

static void *
_callback_null_1 (void  *argp, struct svc_req *rqstp)
{
	return (callback_null_1_svc(rqstp));
}

static void *
_stdio_1 (RPC_Message  *argp, struct svc_req *rqstp)
{
	return (stdio_1_svc(*argp, rqstp));
}

static void
callback_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		RPC_Message stdio_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case CALLBACK_NULL:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) _callback_null_1;
		break;

	case STDIO:
		_xdr_argument = (xdrproc_t) xdr_RPC_Message;
		_xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) _stdio_1;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}

int main(int _argc, char** _argv) {
	server_port = 0;
	
	pmap_unset(CALLBACK, CALLBACK_APPLE);
		
	SVCXPRT* transp = svcudp_create(RPC_ANYSOCK);
	server_port = transp->xp_port;
		
	printf("Callback service on %d\n", server_port);
	if(transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.\n");
		exit(1);
	}

	FILE* temp = stderr;
	stderr = fopen("/dev/null","w"); //not print stderr message
	svc_register(transp, CALLBACK, CALLBACK_APPLE, callback_1, IPPROTO_UDP);
	fclose(stderr);
	stderr = temp;

	cmd_init();

	void execute_cmd(char* line, bool is_dump) {
		//if is_dump == true then file cmd
		//   is_dump == false then stdin cmd
		if(is_dump == true)
			printf("%s\n", line);
		
		int exit_status = cmd_exec(line);
		if(exit_status < 0) {
			if(exit_status == -100)
				printf("wrong number of arguments\n");
			else
				printf("%d'std argument type wrong\n", -exit_status);
		}
			
		printf("> ");
		fflush(stdout);
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
		int seek = 0;
		static int eod = 0; //end of data
		char* search_point;

		while(1) {
			if((eod += read(fd, &line[eod], MAX_LINE_SIZE - eod - 1)) != 0) {
				seek = 0;
				line[eod] = '\0';
				while(1) {
					if((search_point = strchr(&line[seek], '\n')) != NULL) { //find	
						*search_point = '\0';
						execute_cmd(&line[seek], fd != STDIN_FILENO);
						seek = search_point - line + 1;
						continue;
					} else if(seek == 0){ //not found '\n' and seek == 0
						line[eod] = '\0'; //set line
						execute_cmd(&line[seek], fd != STDIN_FILENO);
						seek = 0;
						eod = 0; 
						if(fd == STDIN_FILENO) {
							return;
						} else
							break;
					} else { //not found '\n' and seek != 0
						memmove(line, line + seek, eod - seek);

						eod = eod - seek;
						if(fd == STDIN_FILENO) {
							return;
						} else
							break;
					}
				}
			} else { //not have read data == EOF
				if(eod != 0) {//has last line
					line[eod] = '\0';
					execute_cmd(&line[0], fd != STDIN_FILENO);
				}
				seek = 0;
				eod = 0;
				return;
			}		
		}
	}

	int retval;
	fd_set in_put;
	fd_set temp_in;

	int svc_sock = transp->xp_sock;
	int fd = (int)(int64_t)list_remove_first(fd_list);
	FD_ZERO(&in_put);
	FD_SET(svc_sock, &in_put);
	FD_SET(fd, &in_put);

	printf("> ");
	fflush(stdout);
	while(is_continue) {
		temp_in = in_put;

		retval = select(fd > svc_sock ? (fd + 1) : (svc_sock + 1), &temp_in, 0, 0, NULL);

		if(retval == -1) {
			printf("selector error \n");
			return -1;
		} else if(retval == 0) {
			//select time over
			continue;
		} else {
			if(FD_ISSET(svc_sock, &temp_in) != 0) {
				svc_getreqset(&temp_in);
			}
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
	}
	return 0;
}
