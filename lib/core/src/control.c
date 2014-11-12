#include <stdio.h>
#include <stdbool.h>
#include <poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <errno.h>
#include <rpc/pmap_clnt.h>
#include <control/control.h>

#include "rpc_manager.h"
#include "rpc_callback.h"

static CLIENT *client;
static CLIENT *ioclient;

static char* client_host;
static int server_port;

static SVCXPRT* transp;

static void (*stdout_func)(uint64_t, uint32_t, char*, int);
static void (*stderr_func)(uint64_t, uint32_t, char*, int);
static void (*disconnected)(void);
static void(*progress)(uint64_t vmid, uint32_t progress, uint32_t total);

static uint64_t vmid;

static void *
_callback_null_1 (void *argp, struct svc_req *rqstp)
{
	return (callback_null_1_svc(rqstp));
}

static void *
_stdio_1 (RPC_Message *argp, struct svc_req *rqstp)
{	
	if(argp->fd == 1) {
		if(stdout_func != NULL)
			stdout_func(argp->vmid, argp->coreid, argp->message.message_val, argp->message.message_len);
	} else if(argp->fd == 2) {
		if(stderr_func != NULL)
			stderr_func(argp->vmid, argp->coreid, argp->message.message_val, argp->message.message_len);
	}
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

bool ctrl_connect(char* host, int port, void(*connected)(bool result), void(*_disconnected)(void)) {
	/*
	if(client != NULL) {
		ctrl_disconnect();
	}
	*/
	pmap_unset(CALLBACK, CALLBACK_APPLE);

	transp = svcudp_create(RPC_ANYSOCK);
	if(transp == NULL) {
		errno = -12;
		return false;
	}
	server_port = transp->xp_port;

	FILE* temp = stderr;
	stderr = fopen("/dev/null","w"); //not print stderr message
	svc_register(transp, CALLBACK, CALLBACK_APPLE, callback_1, IPPROTO_UDP);
	fclose(stderr);
	stderr = temp;

	if(_disconnected != NULL)
		disconnected = _disconnected;

	struct hostent* hostent = gethostbyname(host);
	if(!hostent) {
		errno = -4;
		connected(false);
		return false;
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
		errno = -5;
		connected(false);
		return false;
	}

	void* ret = manager_null_1(client);
	if(ret == NULL) {
		client = NULL;
		errno = -6;
		connected(false);
		return false;
	}

	ioclient = clntudp_create(&addr, CALLBACK, CALLBACK_APPLE, time, &sock);
	if(!ioclient) {
		errno = -5;
		connected(false);
		return false;
	}

	ret = callback_null_1(ioclient);
	if(ret == NULL) {
		errno = -6;
		client = NULL;
		connected(false);
		return false;
	}
	RPC_Address cbaddr;
	cbaddr.ip = 0;
	cbaddr.port = server_port;

	bool_t* ret2 = callback_add_1(cbaddr, client);
	if(ret2 == NULL || !*ret2) {
		errno = -7;
		connected(false);
		return false;
	}

	if(client_host) {
		free(client_host);
	}
	int len = strlen(host) + 1;
	client_host = (char*)malloc(len);
	memcpy(client_host, host, len);

	connected(true);

	return true;
}

bool ctrl_disconnect() {
	if(disconnected != NULL)
		disconnected();

	RPC_Address cbaddr;
	cbaddr.ip = 0;
	cbaddr.port = server_port;

	callback_remove_1(cbaddr, client);
	
	if(client != NULL) {
		clnt_destroy(client);
		client = NULL;
	}

	if(ioclient != NULL) {
		clnt_destroy(ioclient);
		ioclient = NULL;
	}

	svc_unregister(CALLBACK, CALLBACK_APPLE);

	if(transp != NULL) {
		svc_destroy(transp);
		transp = NULL;
	}

	disconnected = NULL;
	stdout_func = NULL;
	stderr_func = NULL;
	progress = NULL;
	return true;
}

bool ctrl_ping(int count, void(*callback)(int count, clock_t delay)) {
	if(!client) {
		errno = -2;
		return false;
	}
	
	for(int i = 0; i < count; i++) {
		clock_t time = clock();
		void* ret = manager_null_1(client);
		if(ret != NULL) {
			callback(i, clock() - time);
		} else {
			callback(i, -1);
		}
	}
	return true;
}

bool ctrl_vm_create(VMSpec* vms, void(*callback)(uint64_t vmid)) {
	if(!client) {
		errno = -2;
		return false;
	}

	RPC_VM vm;
	vm.core_num = vms->core_size;
	vm.memory_size = vms->memory_size;
	vm.storage_size = vms->storage_size;

	vm.nics.nics_len = vms->nic_size;
	RPC_NIC nics[RPC_MAX_NIC_COUNT];
	vm.nics.nics_val = nics;

	for(int i = 0; i < vms->nic_size; i++) {
		nics[i].mac = vms->nics[i].mac;
		nics[i].port = vms->nics[i].port;
		nics[i].input_buffer_size = vms->nics[i].input_buffer_size;
		nics[i].output_buffer_size = vms->nics[i].output_buffer_size;
		nics[i].input_bandwidth = vms->nics[i].input_bandwidth;
		nics[i].output_bandwidth = vms->nics[i].output_bandwidth;
		nics[i].pool_size = vms->nics[i].pool_size;
	}

	vm.args.args_len = 0;
	char _args[RPC_MAX_ARGS_SIZE];
	vm.args.args_val = _args;
	
	u_quad_t* vmid = vm_create_1(vm, client);
	if(vmid == NULL) {
		errno = -1;
		ctrl_disconnect();
		return false;
	}

	callback(*vmid);

	return true;
}

bool ctrl_vm_get(uint64_t vmid, void(*callback)(uint64_t vmid, VMSpec* vm)) {
	//not yet
	return true;
}

bool ctrl_vm_set(uint64_t vmid, VMSpec* vm, void(*callback)(bool result)) {
	//not yet
	return true;
}

bool ctrl_vm_delete(uint64_t vmid, void(*callback)(uint64_t vmid, bool result)) {
	if(!client) {
		errno = -2;
		return false;
	}

	bool_t* ret = vm_delete_1(vmid, client);
	if(ret == NULL) {
		errno = -1;
		ctrl_disconnect();
		return false;
	}

	callback(vmid, *ret);

	return true;
}

bool ctrl_vm_list(void(*callback)(uint64_t* vmids, int count)) {
	if(!client) {
		errno = -2;
		return false;
	}

	RPC_VMList* ret = vm_list_1(client);
	if(ret == NULL) {
		errno = -1;
		ctrl_disconnect();
		return false;
	}

	callback(ret->RPC_VMList_val, ret->RPC_VMList_len);

	return true;
}

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	if(progress != NULL) {
		progress(vmid, (uint32_t)ulnow, (uint32_t)ultotal);
	}
	
	return 0;
}

bool ctrl_storage_upload(uint64_t _vmid, char* path, void(*_progress)(uint64_t vmid, uint32_t progress, uint32_t total)) {
	if(!client) {
		errno = -2;
		return false;
	}
	
	curl_global_init(CURL_GLOBAL_NOTHING);
	CURL* curl = curl_easy_init();
	if(!curl) {
		errno = -8;
		return false;
	}
	
	FILE* file = fopen(path, "r");
	if(file == NULL) {
		errno = -9;
		return false;
	}

	struct stat stat;
	if(fstat(fileno(file), &stat) < 0) {
		errno = -10;
		return false;
	}

	vmid = _vmid;
	progress = _progress;

	char url[1024];
	snprintf(url, 1024, "tftp://%s/%ld\n", client_host, vmid);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, file);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)stat.st_size);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	CURLcode res;
	if((res = curl_easy_perform(curl)) != CURLE_OK) {
		errno = -10;
		return false;
	}
	curl_easy_cleanup(curl);
	fclose(file);

	return true;
}

bool ctrl_storage_md5(uint64_t vmid, uint64_t size, void(*callback)(uint64_t vmid, uint32_t md5[4])) {
	if(!client) {
		errno = -2;
		return false;
	}

	RPC_Digest* ret = storage_digest_1(vmid, RPC_MD5, size, client);

	if(ret == NULL) {
		errno = -1;
		ctrl_disconnect();
		return false;
	}

	if(ret->RPC_Digest_len <= 0) {
		errno = -11;
		return false;
	} else {
		uint32_t md5[4];
		memcpy(md5, ret->RPC_Digest_val, sizeof(uint32_t) * 4);
		callback(vmid, md5);
	}
	return true;
}

bool ctrl_status_set(uint64_t vmid, int status, void(*callback)(uint64_t vmid, int status)) {
	if(!client) {
		errno = -2;
		return false;
	}
	
	int vm_status;
	switch(status) {
		case VM_STATUS_START:
			vm_status = RPC_START;
			break;
		case VM_STATUS_PAUSE:
			vm_status = RPC_PAUSE;
			break;
		case VM_STATUS_STOP:
			vm_status = RPC_STOP;
			break;
		case VM_STATUS_RESUME:
			vm_status = RPC_RESUME;
			break;
		default:
			vm_status = RPC_NONE;
	}

	bool_t* ret = status_set_1(vmid, vm_status, client);
	if(ret == NULL) {
		errno = -1;
		ctrl_disconnect();
		return false;
	}

	if(*ret == true) {
		callback(vmid, status);
	} else {
		callback(vmid, VM_STATUS_INVALID);
	}

	return true;

}

bool ctrl_status_get(uint64_t vmid, void(*callback)(uint64_t vmid, int status)) {
	if(!client) {
		errno = -2;
		return false;
	}

	RPC_VMStatus* status = status_get_1(vmid, client);
	if(status == NULL) {
		errno = -1;
		ctrl_disconnect();
		return false;
	}

	int vm_status;
	switch(*status) {
		case RPC_START:
			vm_status = VM_STATUS_START;
			break;
		case RPC_PAUSE:
			vm_status = VM_STATUS_PAUSE;
			break;
		case RPC_STOP:
			vm_status = VM_STATUS_STOP;
			break;
		default:
			vm_status = VM_STATUS_INVALID;
	}

	callback(vmid, vm_status);

	return true;
}

bool ctrl_stdout(void(*out)(uint64_t vmid, uint32_t thread_id, char* msg, int count)) {
	stdout_func = out;
	return true;
}

bool ctrl_stderr(void(*err)(uint64_t vmid, uint32_t thread_id, char* msg, int count)) {
	stderr_func = err;
	return true;
}

bool ctrl_stdin(uint64_t vmid, uint32_t thread_id, char* msg, int count) {
	// not yet
	return true;
}

bool ctrl_poll() {
	if(!client) {
		errno = -2;
		return false;
	}

	struct pollfd ctrl_fd;
	int state;
	ctrl_fd.fd = transp->xp_sock;
	ctrl_fd.events = POLLIN;
	ctrl_fd.revents = 0;

	state = poll(&ctrl_fd, 1, 50);
	if(state > 0) {
		switch(state) {
			case -1:
				errno = -3;
				ctrl_disconnect();
				return false;
			case 0:
				break;
			default:
				if(ctrl_fd.revents & POLLIN) {
					svc_getreq_poll(&ctrl_fd, state);
				}
				break;
		}
	}
	return true;
}
