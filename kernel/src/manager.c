#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <rpc/svc.h>
#include <net/ether.h>
#undef IP_TTL
#include <net/ip.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/rpc.h>
#include <net/tftp.h>
#include <util/map.h>
#include <util/ring.h>
#include <util/event.h>
#include "malloc.h"
#include "mp.h"
#include "loader.h"
#include "cpu.h"
#include "vm.h"
#include "shell.h"
#include "rpc/rpc_manager.h"
#include "rpc/rpc_callback.h"

#include "manager.h"

#define DEFAULT_MANAGER_IP	0xc0a864fe	// 192.168.100.254
#define DEFAULT_MANAGER_GW	0xc0a864c8	// 192.168.100.200
#define DEFAULT_MANAGER_NM	0xffffff00	// 255.255.255.0

NI*	manager_ni;

typedef struct {
	uint32_t	addr;
	uint16_t	port;
	CLIENT*		client;
	uint64_t	time;
} Callback;

static List* callbacks;

// RPC
void * manager_null_1_svc(struct svc_req *rqstp) {
	static void* result;
	return (void*)&result;
}

u_quad_t * vm_create_1_svc(RPC_VM rpc_vm,  struct svc_req *rqstp) {
	static u_quad_t result;
	
	VMSpec* vm = malloc(sizeof(VMSpec));
	vm->core_size = rpc_vm.core_num;
	vm->memory_size = rpc_vm.memory_size;
	vm->storage_size = rpc_vm.storage_size;
	vm->nic_size = rpc_vm.nics.nics_len;
	vm->nics = malloc(sizeof(NICSpec) * vm->nic_size);
	for(int i = 0; i < vm->nic_size; i++) {
		vm->nics[i].mac = rpc_vm.nics.nics_val[i].mac;
		vm->nics[i].input_buffer_size = rpc_vm.nics.nics_val[i].input_buffer_size;
		vm->nics[i].output_buffer_size = rpc_vm.nics.nics_val[i].output_buffer_size;
		vm->nics[i].input_bandwidth = rpc_vm.nics.nics_val[i].input_bandwidth;
		vm->nics[i].output_bandwidth = rpc_vm.nics.nics_val[i].output_bandwidth;
		vm->nics[i].pool_size = rpc_vm.nics.nics_val[i].pool_size;
	}
	vm->argc = 0;
	char* args = rpc_vm.args.args_val;
	for(int i = 0; i < rpc_vm.args.args_len; i++) {
		if(args[i] == '\0')
			vm->argc++;
	}
	vm->argv = malloc(sizeof(char*) * vm->argc);
	vm->argv[0] = args;
	for(int i = 0, j = 1; i < rpc_vm.args.args_len - 1; i++) {
		if(args[i] == '\0')
			vm->argv[j++] = args + i + 1;
	}
	
	result = vm_create(vm);
	
	free(vm->argv);
	free(vm->nics);
	free(vm);
	
	return &result;
}

RPC_VM* vm_get_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
	static RPC_VM result;
	
	/*
	 * insert server code here
	 */

	return &result;
}

bool_t* vm_set_1_svc(u_quad_t vmid, RPC_VM vm,  struct svc_req *rqstp) {
	static bool_t result;
	result = FALSE;

	/*
	 * insert server code here
	 */

	return &result;
}

bool_t * vm_delete_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
	static bool_t result;
	
	result = vm_delete(vmid);
	
	return &result;
}

RPC_VMList * vm_list_1_svc(struct svc_req *rqstp) {
	static RPC_VMList result;
	static u_quad_t vmids[RPC_MAX_VM_COUNT];
	
	result.RPC_VMList_val = vmids;
	result.RPC_VMList_len = vm_list(vmids, RPC_MAX_VM_COUNT);
	
	return &result;
}

bool_t* status_set_1_svc(u_quad_t vmid, RPC_VMStatus status, struct svc_req *rqstp) {
	void status_setted(bool result, void* context) {
		SVCXPRT* transp = context;
		
		bool_t result2 = result;
		if(!svc_sendreply(transp, (xdrproc_t)xdr_bool, (caddr_t)&result2)) {
			svcerr_systemerr(transp);
		}
		free(transp);
	}
	
	SVCXPRT* transp = malloc(sizeof(SVCXPRT));
	memcpy(transp, rqstp->rq_xprt, sizeof(SVCXPRT));
	
	int status2 = 0;
	switch(status) {
		case RPC_START: status2 = VM_STATUS_START; break;
		case RPC_PAUSE: status2 = VM_STATUS_PAUSE; break;
		case RPC_RESUME: status2 = VM_STATUS_RESUME; break;
		case RPC_STOP: status2 = VM_STATUS_STOP; break;
		default:
			free(transp);
			svcerr_systemerr(rqstp->rq_xprt);
			return NULL;
	}
	
	vm_status_set(vmid, status2, status_setted, transp);
	
	return NULL;	// Async call
}

RPC_VMStatus * status_get_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
	static RPC_VMStatus  result;

	switch(vm_status_get(vmid)) {
		case VM_STATUS_STOP:
			result = RPC_STOP;
			break;
		case VM_STATUS_PAUSE:
			result = RPC_PAUSE;
			break;
		case VM_STATUS_START:
			result = RPC_START;
			break;
		default:
			result = RPC_NONE;
			break;
	}
	
	return &result;
}

RPC_Digest * storage_digest_1_svc(u_quad_t vmid, RPC_MessageDigestType type, u_quad_t size,  struct svc_req *rqstp) {
	static RPC_Digest  result;
	static uint32_t md5sum[4];
	
	result.RPC_Digest_len = 16;
	result.RPC_Digest_val = (char*)md5sum;
	
	if(!vm_storage_md5(vmid, size, md5sum))
		result.RPC_Digest_len = 0;
	
	return &result;
}

// Callback RPC
static uint32_t get_remote_addr(struct svc_req* rqstp) {
	Packet* packet = (Packet*)rqstp->rq_xprt->xp_p2;
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	
	return endian32(ip->source);
}

static Callback* callback_create(uint32_t addr, uint16_t port) {
	ListIterator iter;
	list_iterator_init(&iter, callbacks);
	while(list_iterator_has_next(&iter)) {
		Callback* c = list_iterator_next(&iter);
		if(c->addr == addr && c->port == port) {
			c->time = cpu_tsc();
			return NULL;
		}
	}
	
	Callback* cb = malloc(sizeof(Callback));
	if(!cb)
		return NULL;
	
	bzero(cb, sizeof(Callback));
	cb->addr = addr;
	cb->port = port;
	cb->client = rpc_client(cb->addr, cb->port, CALLBACK, CALLBACK_APPLE);
	cb->time = cpu_tsc();
	
	if(!list_add(callbacks, cb)) {
		clnt_destroy(cb->client);
		free(cb);
		return NULL;
	}
	
	return cb;
}

bool_t * callback_add_1_svc(RPC_Address addr, struct svc_req *rqstp) {
	static bool_t  result;
	result = FALSE;
	
	uint32_t ip = addr.ip == 0 ? get_remote_addr(rqstp) : addr.ip;
	Callback* cb = callback_create(ip, addr.port);
	if(!cb)
		return &result;
	
	result = TRUE;
	return &result;
}

bool_t * callback_remove_1_svc(RPC_Address addr,  struct svc_req *rqstp) {
	static bool_t  result;
	result = FALSE;
	
	uint32_t ip = addr.ip == 0 ? get_remote_addr(rqstp) : addr.ip;
	
	ListIterator iter;
	list_iterator_init(&iter, callbacks);
	while(list_iterator_has_next(&iter)) {
		Callback* cb = list_iterator_next(&iter);
		
		if(cb->addr == ip && cb->port == addr.port) {
			list_iterator_remove(&iter);
			free(cb);
			
			result = TRUE;
			break;
		}
	}
	
	return &result;
}

RPC_Addresses * callback_list_1_svc(struct svc_req *rqstp) {
	static RPC_Addresses result;
	static RPC_Address addrs[RPC_MAX_CALLBACK_COUNT];
	
	result.RPC_Addresses_len = list_size(callbacks);
	result.RPC_Addresses_val = addrs;
	
	int i = 0;
	ListIterator iter;
	list_iterator_init(&iter, callbacks);
	while(list_iterator_has_next(&iter)) {
		Callback* cb = list_iterator_next(&iter);
		addrs[i].ip = cb->addr;
		addrs[i].port = cb->port;
		i++;
	}
	
	return &result;
}

void * callback_null_1_svc(struct svc_req *rqsp) {
	static void* result;
	return (void*)&result;
}

void * stdio_1_svc(RPC_Message msg, struct svc_req *rqsp) {
	static void* result;
	// TODO: Implement it
	printf("%d %d %d %s", msg.vmid, msg.coreid, msg.fd, msg.message.message_val);
	
	return (void*)&result;
}

// TFTP
static int tftp_create(char* filename, int mode) { // 1: read, 2: write
	uint64_t vmid = (uint64_t)strtoll(filename, NULL, 0);
	if(vm_contains(vmid)) {
		printf("TFTP: %d [");
		return (int)vmid;	// TODO: Change vmid type to int
	} else {
		return -1;
	}
}

static int tftp_write(int fd, void* buf, uint32_t offset, int size) {
	uint64_t vmid = (uint64_t)fd;
	if(!vm_contains(vmid))
		return -2;
	
	if(vm_storage_write(vmid, buf, offset, size) != size)
		return -1;
	
	if(size == 512)
		printf(".");
	else
		printf("]\n");
	
	return size;
}

static int tftp_read(int fd, void* buf, uint32_t offset, int size) {
	uint64_t vmid = (uint64_t)fd;
	if(!vm_contains(vmid))
		return -2;
	
	if(vm_storage_read(vmid, buf, offset, size) != size)
		return -1;
	
	return size;
}

static TFTPCallback tftp_callback = { tftp_create, tftp_write, tftp_read };

// RPC Manager
static bool manager_loop(NetworkInterface* ni) {
	// Packet processing
	Packet* packet = ni_input(ni);
	if(packet) {
		if(shell_process(packet))
			;
		else if(arp_process(packet))
			;
		else if(icmp_process(packet))
			;
		else if(rpc_process(packet))
			;
		else if(tftp_process(packet))
			;
		else
			ni_free(packet);
	}
	
	// GC callbacks
	/* TODO: Activate it after interval callback_add call from console
	static uint64_t gc_callback;
	uint64_t time = cpu_tsc();
	if(gc_callback < time) {
		uint64_t timeout = time + 10 * cpu_frequency;
		
		ListIterator iter;
		list_iterator_init(&iter, callbacks);
		while(list_iterator_has_next(&iter)) {
			Callback* cb = list_iterator_next(&iter);
			if(cb->time < timeout) {
				list_iterator_remove(&iter);
				clnt_destroy(cb->client);
				free(cb);
			}
		}
		
		gc_callback = time + 2 * cpu_frequency;
	}
	*/
	
	/*
	static uint64_t debug_timer;
	if(debug_timer < cpu_tsc()) {
		printf("memory: local: %ld/%ldKiB, global: %ld/%ldKiB, block: %ld/%ldKiB\n", 
			malloc_used() / 1024, malloc_total() / 1024,
			gmalloc_used() / 1024, gmalloc_total() / 1024,
			bmalloc_used() / 1024, bmalloc_total() / 1024);
		
		debug_timer = cpu_tsc() + 3 * cpu_frequency;
	}
	*/
	
	
	return true;
}

static void stdio_callback(uint64_t vmid, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
	RPC_Message msg;
	msg.vmid = vmid;
	msg.coreid = thread_id;
	msg.fd = fd;
	
	#define MAX_MESSAGE (1500 - 14 - 20 - 8 - 62)
	
	int text_len = ring_readable(*head, *tail, size);
	if(text_len > MAX_MESSAGE) {
		text_len = MAX_MESSAGE;
	}
	
	char* buf = malloc(text_len);
	ring_read(buffer, head, *tail, size, buf, text_len);
	
	if(vmid == 0) {
		printf("Core %d%c ", thread_id, fd == 1 ? '>' : '!');
		write(1, buf, text_len);
	}
	
	msg.message.message_len = text_len;
	msg.message.message_val = buf;
	
	ListIterator iter;
	list_iterator_init(&iter, callbacks);
	while(list_iterator_has_next(&iter)) {
		Callback* cb = list_iterator_next(&iter);
		if(!rpc_call_async(manager_ni->ni, cb->client, STDIO, (xdrproc_t)xdr_RPC_Message, (char*)&msg))
			printf("Cannot send Std I/O message: %d\n", errno);
	}
	free(buf);
}

void manager_init() {
	uint64_t attrs[] = { 
		NI_MAC, ni_mac, // Physical MAC
		NI_POOL_SIZE, 0x400000,
		NI_INPUT_BANDWIDTH, 1000000000L,
		NI_OUTPUT_BANDWIDTH, 1000000000L,
		NI_INPUT_BUFFER_SIZE, 1024,
		NI_OUTPUT_BUFFER_SIZE, 1024,
		NI_INPUT_ACCEPT_ALL, 1,
		NI_OUTPUT_ACCEPT_ALL, 1,
		NI_NONE
	};
	
	manager_ni = ni_create(attrs);
	ni_config_put(manager_ni->ni, "ip", (void*)(uint64_t)DEFAULT_MANAGER_IP);
	ni_config_put(manager_ni->ni, "gateway", (void*)(uint64_t)DEFAULT_MANAGER_GW);
	ni_config_put(manager_ni->ni, "netmask", (void*)(uint64_t)DEFAULT_MANAGER_NM);
	ni_config_put(manager_ni->ni, TFTP_CALLBACK, &tftp_callback);
	
	callbacks = list_create(NULL);
	
	SVCXPRT* rpc;
	if(!(rpc = svcudp_create(111)))
		printf("Cannot create service\n");
	
	void manager_1(struct svc_req *rqstp, register SVCXPRT *transp);
	if(!svc_register(rpc, MANAGER, MANAGER_APPLE, manager_1, IPPROTO_UDP))
		printf("Cannot register manager service\n");
	
	void callback_1(struct svc_req *rqstp, register SVCXPRT *transp);
	if(!svc_register(rpc, CALLBACK, CALLBACK_APPLE, callback_1, IPPROTO_UDP))
		printf("Cannot register callback service\n");
	
	event_idle_add((void*)manager_loop, manager_ni->ni);
	
	vm_stdio(stdio_callback);
}

uint32_t manager_get_ip() {
	return (uint32_t)(uint64_t)ni_config_get(manager_ni->ni, "ip");
}

void manager_set_ip(uint32_t ip) {
	ni_config_put(manager_ni->ni, "ip", (void*)(uint64_t)ip);
}

uint32_t manager_get_gateway() {
	return (uint32_t)(uint64_t)ni_config_get(manager_ni->ni, "gateway");
}

void manager_set_gateway(uint32_t gw) {
	ni_config_put(manager_ni->ni, "gateway", (void*)(uint64_t)gw);
}

uint32_t manager_get_netmask() {
	return (uint32_t)(uint64_t)ni_config_get(manager_ni->ni, "netmask");
}

void manager_set_netmask(uint32_t nm) {
	ni_config_put(manager_ni->ni, "netmask", (void*)(uint64_t)nm);
}
