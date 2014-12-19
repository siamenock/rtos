#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <net/ether.h>
#undef IP_TTL
#include <net/ip.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/tftp.h>
#include <util/list.h>
#include <util/ring.h>
#include <util/event.h>
#undef BYTE_ORDER
#include <lwip/tcp.h>
#include <control/rpc.h>
#include "malloc.h"
#include "mp.h"
#include "cpu.h"
#include "shell.h"
#include "vm.h"

#include "manager.h"

#define DEFAULT_MANAGER_IP	0xc0a864fe	// 192.168.100.254
#define DEFAULT_MANAGER_GW	0xc0a864c8	// 192.168.100.200
#define DEFAULT_MANAGER_NETMASK	0xffffff00	// 255.255.255.0
#define DEFAULT_MANAGER_PORT	111

NI*	manager_ni;

//typedef struct {
//	uint32_t	addr;
//	uint16_t	port;
//	CLIENT*		client;
//	uint64_t	time;
//} Callback;
//
//static List* callbacks;
//
//// RPC
//void * manager_null_1_svc(struct svc_req *rqstp) {
//	static void* result;
//	return (void*)&result;
//}
//
//u_quad_t * vm_create_1_svc(RPC_VM rpc_vm,  struct svc_req *rqstp) {
//	static u_quad_t result;
//	
//	VMSpec* vm = malloc(sizeof(VMSpec));
//	vm->core_size = rpc_vm.core_num;
//	vm->memory_size = rpc_vm.memory_size;
//	vm->storage_size = rpc_vm.storage_size;
//	vm->nic_size = rpc_vm.nics.nics_len;
//	vm->nics = malloc(sizeof(NICSpec) * vm->nic_size);
//	for(int i = 0; i < vm->nic_size; i++) {
//		vm->nics[i].mac = rpc_vm.nics.nics_val[i].mac;
//		vm->nics[i].input_buffer_size = rpc_vm.nics.nics_val[i].input_buffer_size;
//		vm->nics[i].output_buffer_size = rpc_vm.nics.nics_val[i].output_buffer_size;
//		vm->nics[i].input_bandwidth = rpc_vm.nics.nics_val[i].input_bandwidth;
//		vm->nics[i].output_bandwidth = rpc_vm.nics.nics_val[i].output_bandwidth;
//		vm->nics[i].pool_size = rpc_vm.nics.nics_val[i].pool_size;
//	}
//	vm->argc = 0;
//	char* args = rpc_vm.args.args_val;
//	for(int i = 0; i < rpc_vm.args.args_len; i++) {
//		if(args[i] == '\0')
//			vm->argc++;
//	}
//	vm->argv = malloc(sizeof(char*) * vm->argc);
//	
//	if(rpc_vm.args.args_len > 0) {
//		vm->argv[0] = args;
//		
//		for(int i = 0, j = 1; i < rpc_vm.args.args_len - 1; i++) {
//			if(args[i] == '\0')
//				vm->argv[j++] = args + i + 1;
//		}
//	}
//	
//	result = vm_create(vm);
//	
//	free(vm->argv);
//	free(vm->nics);
//	free(vm);
//	
//	return &result;
//}
//
//RPC_VM* vm_get_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
//	static RPC_VM result;
//	
//	/*
//	 * insert server code here
//	 */
//
//	return &result;
//}
//
//bool_t* vm_set_1_svc(u_quad_t vmid, RPC_VM vm,  struct svc_req *rqstp) {
//	static bool_t result;
//	result = FALSE;
//
//	/*
//	 * insert server code here
//	 */
//
//	return &result;
//}
//
//bool_t * vm_delete_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
//	static bool_t result;
//	
//	result = vm_delete(vmid);
//	
//	return &result;
//}
//
//RPC_VMList * vm_list_1_svc(struct svc_req *rqstp) {
//	static RPC_VMList result;
//	static u_quad_t vmids[RPC_MAX_VM_COUNT];
//	
//	result.RPC_VMList_val = vmids;
//	result.RPC_VMList_len = vm_list(vmids, RPC_MAX_VM_COUNT);
//	
//	return &result;
//}
//
//bool_t* status_set_1_svc(u_quad_t vmid, RPC_VMStatus status, struct svc_req *rqstp) {
//	void status_setted(bool result, void* context) {
//		SVCXPRT* transp = context;
//		
//		bool_t result2 = result;
//		if(!svc_sendreply(transp, (xdrproc_t)xdr_bool, (caddr_t)&result2)) {
//			svcerr_systemerr(transp);
//		}
//		free(transp);
//	}
//	
//	SVCXPRT* transp = malloc(sizeof(SVCXPRT));
//	memcpy(transp, rqstp->rq_xprt, sizeof(SVCXPRT));
//	
//	int status2 = 0;
//	switch(status) {
//		case RPC_START: status2 = VM_STATUS_START; break;
//		case RPC_PAUSE: status2 = VM_STATUS_PAUSE; break;
//		case RPC_RESUME: status2 = VM_STATUS_RESUME; break;
//		case RPC_STOP: status2 = VM_STATUS_STOP; break;
//		default:
//			free(transp);
//			svcerr_systemerr(rqstp->rq_xprt);
//			return NULL;
//	}
//	
//	vm_status_set(vmid, status2, status_setted, transp);
//	
//	return NULL;	// Async call
//}
//
//RPC_VMStatus * status_get_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
//	static RPC_VMStatus  result;
//
//	switch(vm_status_get(vmid)) {
//		case VM_STATUS_STOP:
//			result = RPC_STOP;
//			break;
//		case VM_STATUS_PAUSE:
//			result = RPC_PAUSE;
//			break;
//		case VM_STATUS_START:
//			result = RPC_START;
//			break;
//		default:
//			result = RPC_NONE;
//			break;
//	}
//	
//	return &result;
//}
//
//RPC_Digest * storage_digest_1_svc(u_quad_t vmid, RPC_MessageDigestType type, u_quad_t size,  struct svc_req *rqstp) {
//	static RPC_Digest  result;
//	static uint32_t md5sum[4];
//	
//	result.RPC_Digest_len = 16;
//	result.RPC_Digest_val = (char*)md5sum;
//	
//	if(!vm_storage_md5(vmid, size, md5sum))
//		result.RPC_Digest_len = 0;
//	
//	return &result;
//}
//
//// Callback RPC
//static uint32_t get_remote_addr(struct svc_req* rqstp) {
//	Packet* packet = (Packet*)rqstp->rq_xprt->xp_p2;
//	Ether* ether = (Ether*)(packet->buffer + packet->start);
//	IP* ip = (IP*)ether->payload;
//	
//	return endian32(ip->source);
//}
//
//static Callback* callback_create(uint32_t addr, uint16_t port) {
//	ListIterator iter;
//	list_iterator_init(&iter, callbacks);
//	while(list_iterator_has_next(&iter)) {
//		Callback* c = list_iterator_next(&iter);
//		if(c->addr == addr && c->port == port) {
//			c->time = cpu_tsc();
//			return NULL;
//		}
//	}
//	
//	Callback* cb = malloc(sizeof(Callback));
//	if(!cb)
//		return NULL;
//	
//	bzero(cb, sizeof(Callback));
//	cb->addr = addr;
//	cb->port = port;
//	cb->client = rpc_client(cb->addr, cb->port, CALLBACK, CALLBACK_APPLE);
//	cb->time = cpu_tsc();
//	
//	if(!list_add(callbacks, cb)) {
//		clnt_destroy(cb->client);
//		free(cb);
//		return NULL;
//	}
//	
//	return cb;
//}
//
//bool_t * callback_add_1_svc(RPC_Address addr, struct svc_req *rqstp) {
//	static bool_t  result;
//	result = FALSE;
//	
//	uint32_t ip = addr.ip == 0 ? get_remote_addr(rqstp) : addr.ip;
//	Callback* cb = callback_create(ip, addr.port);
//	if(!cb)
//		return &result;
//	
//	result = TRUE;
//	return &result;
//}
//
//bool_t * callback_remove_1_svc(RPC_Address addr,  struct svc_req *rqstp) {
//	static bool_t  result;
//	result = FALSE;
//	
//	uint32_t ip = addr.ip == 0 ? get_remote_addr(rqstp) : addr.ip;
//	
//	ListIterator iter;
//	list_iterator_init(&iter, callbacks);
//	while(list_iterator_has_next(&iter)) {
//		Callback* cb = list_iterator_next(&iter);
//		
//		if(cb->addr == ip && cb->port == addr.port) {
//			list_iterator_remove(&iter);
//			free(cb);
//			
//			result = TRUE;
//			break;
//		}
//	}
//	
//	return &result;
//}
//
//RPC_Addresses * callback_list_1_svc(struct svc_req *rqstp) {
//	static RPC_Addresses result;
//	static RPC_Address addrs[RPC_MAX_CALLBACK_COUNT];
//	
//	result.RPC_Addresses_len = list_size(callbacks);
//	result.RPC_Addresses_val = addrs;
//	
//	int i = 0;
//	ListIterator iter;
//	list_iterator_init(&iter, callbacks);
//	while(list_iterator_has_next(&iter)) {
//		Callback* cb = list_iterator_next(&iter);
//		addrs[i].ip = cb->addr;
//		addrs[i].port = cb->port;
//		i++;
//	}
//	
//	return &result;
//}
//
//void * callback_null_1_svc(struct svc_req *rqsp) {
//	static void* result;
//	return (void*)&result;
//}
//
//void * stdio_1_svc(RPC_Message msg, struct svc_req *rqsp) {
//	static void* result;
//	// TODO: Implement it
//	printf("%d %d %d %s", msg.vmid, msg.coreid, msg.fd, msg.message.message_val);
//	
//	return (void*)&result;
//}
//
//// TFTP
//static int tftp_create(char* filename, int mode) { // 1: read, 2: write
//	uint64_t vmid = (uint64_t)strtoll(filename, NULL, 0);
//	if(vm_contains(vmid)) {
//		printf("TFTP: %d [");
//		return (int)vmid;	// TODO: Change vmid type to int
//	} else {
//		return -1;
//	}
//}
//
//static int tftp_write(int fd, void* buf, uint32_t offset, int size) {
//	uint64_t vmid = (uint64_t)fd;
//	if(!vm_contains(vmid))
//		return -2;
//	
//	if(vm_storage_write(vmid, buf, offset, size) != size)
//		return -1;
//	
//	if(size == 512)
//		printf(".");
//	else
//		printf("]\n");
//	
//	return size;
//}
//
//static int tftp_read(int fd, void* buf, uint32_t offset, int size) {
//	uint64_t vmid = (uint64_t)fd;
//	if(!vm_contains(vmid))
//		return -2;
//	
//	if(vm_storage_read(vmid, buf, offset, size) != size)
//		return -1;
//	
//	return size;
//}
//
//static TFTPCallback tftp_callback = { tftp_create, tftp_write, tftp_read };

// LwIP TCP callbacks
typedef struct {
	struct tcp_pcb*	pcb;
	List*		pbufs;
	int		poll_count;
} RPCData;

static List* clients;
static List* actives;

static err_t manager_poll(void* arg, struct tcp_pcb* pcb);

static void manager_close(struct tcp_pcb* pcb, RPC* rpc) {
	printf("Close connection: %p\n", pcb);
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_err(pcb, NULL);
	tcp_poll(pcb, NULL, 0);
	
	if(rpc) {
		RPCData* data = (RPCData*)rpc->data;
		
		ListIterator iter;
		list_iterator_init(&iter, data->pbufs);
		while(list_iterator_has_next(&iter)) {
			struct pbuf* pbuf = list_iterator_next(&iter);
			list_iterator_remove(&iter);
			pbuf_free(pbuf);
		}
		list_destroy(data->pbufs);
		list_remove_data(actives, rpc);
		free(rpc);
	}
	
	if(tcp_close(pcb) != ERR_OK) {
		tcp_poll(pcb, manager_poll, 2);
	}
	
	list_remove_data(clients, pcb);
}

static err_t manager_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
	RPC* rpc = arg;
	
	if(p == NULL) {	// Remote host closed the connection
		manager_close(pcb, rpc);
	} else if(err != ERR_OK) {
		pbuf_free(p);
	} else {
		RPCData* data = (RPCData*)rpc->data;
		list_add(data->pbufs, p);
		
		rpc_loop(rpc);
		tcp_recved(pcb, p->len);
		
		if(rpc_is_active(rpc)) {
			if(list_index_of(actives, rpc, NULL) < 0) {
				list_add(actives, rpc);
			}
		}
	}

	return ERR_OK;
}

static void manager_err(void* arg, err_t err) {
	printf("Error: %d %p\n", err, arg);
	RPC* rpc = arg;
	if(rpc != NULL) {
		RPCData* data = (RPCData*)rpc->data;
		manager_close(data->pcb, rpc);
	}
}

static err_t manager_poll(void* arg, struct tcp_pcb* pcb) {
	RPC* rpc = arg;
	
	if(rpc == NULL) {
		manager_close(pcb, NULL);
	} else {
		RPCData* data = (RPCData*)rpc->data;
		
		if(rpc->ver == 0) {
			data->poll_count++;
		}
		
		if(data->poll_count >= 4) {
			manager_close(pcb, rpc);
		}
	}
	
	return ERR_OK;
}

// Handlers
static uint32_t vm_create_handler(VMSpec* vm, void* context) {
	return vm_create(vm);
}

static VMSpec* vm_get_handler(uint32_t vmid, void* context) {
	// TODO: Implement it
	return NULL;
}

static bool vm_set_handler(VMSpec* vm, void* context) {
	// TODO: Implement it
	return false;
}

static bool vm_delete_handler(uint32_t vmid, void* context) {
	return vm_delete(vmid);
}

static int vm_list_handler(uint32_t* ids, int size, void* context) {
	return vm_list(ids, size);
}

static VMStatus status_get_handler(uint32_t vmid, void* context) {
	return vm_status_get(vmid);
}

static bool status_set_handler(uint32_t vmid, VMStatus status, void* context) {
	void status_setted(bool result, void* context) {
		uint32_t id = (uint32_t)(uint64_t)context;
		printf("Status set: %d %d\n", id, result);
	}
	
	// TODO: Async call
	vm_status_set(vmid, status, status_setted, (void*)(uint64_t)vmid);
	
	return true;
}

static uint16_t storage_download_handler(uint32_t vmid, uint32_t offset, void** buf, uint16_t size, void* context) {
	return vm_storage_read(vmid, buf, offset, size);
}

static uint16_t storage_upload_handler(uint32_t vmid, uint32_t offset, void* buf, uint16_t size, void* context) {
	return vm_storage_write(vmid, buf, offset, size);
}

static uint16_t stdio_handler(uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context) {
	ssize_t len = vm_stdio(id, thread_id, fd, str, size);
	return len < 0 ? 0 : len;
}

// PCB utility
static int pcb_read(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	
	int idx = 0;
	ListIterator iter;
	list_iterator_init(&iter, data->pbufs);
	while(list_iterator_has_next(&iter)) {
		struct pbuf* pbuf = list_iterator_next(&iter);
		if(idx + pbuf->len <= size) {
			memcpy(buf + idx, pbuf->payload, pbuf->len);
			idx += pbuf->len;
			size -= pbuf->len;
			pbuf_free(pbuf);
			list_iterator_remove(&iter);
		} else {
			break;
		}
	}
	
	return idx;
}

static int pcb_write(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	
	int len = tcp_sndbuf(data->pcb);
	len = len > size ? size : len;
	
	if(tcp_write(data->pcb, buf, len, TCP_WRITE_FLAG_COPY) != ERR_OK) {
		errno = -1;
		return -1;
	}
	
	return len;
}

static void pcb_close(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	
	printf("closing: %d\n", errno);
	manager_close(data->pcb, rpc);
}

static err_t manager_accept(void* arg, struct tcp_pcb* pcb, err_t err) {
	struct tcp_pcb_listen* server = arg;
	tcp_accepted(server);
	printf("Accepted: %p\n", pcb);
	
	RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
	bzero(rpc, sizeof(RPC) + sizeof(RPCData));
	rpc->read = pcb_read;
	rpc->write = pcb_write;
	rpc->close = pcb_close;
	
	rpc_vm_create_handler(rpc, vm_create_handler, NULL);
	rpc_vm_get_handler(rpc, vm_get_handler, NULL);
	rpc_vm_set_handler(rpc, vm_set_handler, NULL);
	rpc_vm_delete_handler(rpc, vm_delete_handler, NULL);
	rpc_vm_list_handler(rpc, vm_list_handler, NULL);
	rpc_status_get_handler(rpc, status_get_handler, NULL);
	rpc_status_set_handler(rpc, status_set_handler, NULL);
	rpc_storage_download_handler(rpc, storage_download_handler, NULL);
	rpc_storage_upload_handler(rpc, storage_upload_handler, NULL);
	rpc_stdio_handler(rpc, stdio_handler, NULL);
	
	RPCData* data = (RPCData*)rpc->data;
	data->pcb = pcb;
	data->pbufs = list_create(NULL);
	
	tcp_arg(pcb, rpc);
	tcp_recv(pcb, manager_recv);
	tcp_err(pcb, manager_err);
	tcp_poll(pcb, manager_poll, 2);
	
	list_add(clients, pcb);
	
	return ERR_OK;
}

// RPC Manager
static Packet* manage(Packet* packet) {
	if(shell_process(packet))
		return NULL;
//	else if(rpc_process(packet))
//		return NULL;
//	else if(tftp_process(packet))
//		return NULL;
	
	return packet;
}

static void stdio_callback(uint32_t vmid, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
	ListIterator iter;
	list_iterator_init(&iter, clients);
	while(list_iterator_has_next(&iter)) {
		struct tcp_pcb* pcb = list_iterator_next(&iter);
		RPC* rpc = pcb->callback_arg;
		
		if(*head <= *tail) {
			size_t len0 = *tail - *head;
			
			// TODO: check missed data (via callback);
			rpc_stdio(rpc, vmid, thread_id, fd, buffer + *head, len0, NULL, NULL);
		} else {
			size_t len1 = size - *head;
			size_t len2 = *tail;

			// TODO: check missed data (via callback);
			rpc_stdio(rpc, vmid, thread_id, fd, buffer + *head, len1, NULL, NULL);
			rpc_stdio(rpc, vmid, thread_id, fd, buffer, len2, NULL, NULL);
		}
	}
	
	if(*head <= *tail) {
		size_t len0 = *tail - *head;
		
		*head += len0;
	} else {
		size_t len1 = size - *head;
		size_t len2 = *tail;
		
		*head = (*head + len1 + len2) % size;
	}
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
	ni_config_put(manager_ni->ni, "netmask", (void*)(uint64_t)DEFAULT_MANAGER_NETMASK);
	ni_config_put(manager_ni->ni, "default", (void*)(uint64_t)true);
//	ni_config_put(manager_ni->ni, TFTP_CALLBACK, &tftp_callback);
	
	ni_init(manager_ni->ni, manage, NULL);
	
	struct ip_addr ip;
	IP4_ADDR(&ip, (DEFAULT_MANAGER_IP >> 24) & 0xff, (DEFAULT_MANAGER_IP >> 16) & 0xff, 
		(DEFAULT_MANAGER_IP >> 8) & 0xff, (DEFAULT_MANAGER_IP >> 0) & 0xff);
	
	struct tcp_pcb* server = tcp_new();
	
	err_t err = tcp_bind(server, &ip, DEFAULT_MANAGER_PORT);
	if(err != ERR_OK)
		printf("ERROR: Manager cannot bind TCP session: %d\n", err);
	
	server = tcp_listen(server);
	tcp_arg(server, server);
	
	printf("Manager started: %d.%d.%d.%d:%d\n", 
		(DEFAULT_MANAGER_IP >> 24) & 0xff, (DEFAULT_MANAGER_IP >> 16) & 0xff, 
		(DEFAULT_MANAGER_IP >> 8) & 0xff, (DEFAULT_MANAGER_IP >> 0) & 0xff,
		DEFAULT_MANAGER_PORT);
	
	tcp_accept(server, manager_accept);
	
	clients = list_create(NULL);
	actives = list_create(NULL);
	
	bool manager_loop(void* context) {
		ni_poll();
		
		if(!list_is_empty(actives)) {
			ListIterator iter;
			list_iterator_init(&iter, actives);
			while(list_iterator_has_next(&iter)) {
				RPC* rpc = list_iterator_next(&iter);
				if(rpc_is_active(rpc)) {
					rpc_loop(rpc);
				} else {
					list_iterator_remove(&iter);
				}
			}
		}
		
		return true;
	}
	
	bool manager_timer(void* context) {
		ni_timer();
		
		return true;
	}
	
	event_idle_add(manager_loop, NULL);
	event_timer_add(manager_timer, NULL, 100000, 100000);
	
	vm_stdio_handler(stdio_callback);
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
