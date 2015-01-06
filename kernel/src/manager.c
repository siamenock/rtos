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
static void vm_create_handler(VMSpec* vm, void* context, void(*callback)(uint32_t id)) {
	uint32_t id = vm_create(vm);
	callback(id);
}

static void vm_get_handler(uint32_t vmid, void* context, void(*callback)(VMSpec* vm)) {
	// TODO: Implement it
	callback(NULL);
}

static void vm_set_handler(VMSpec* vm, void* context, void(*callback)(bool result)) {
	// TODO: Implement it
	callback(false);
}

static void vm_delete_handler(uint32_t vmid, void* context, void(*callback)(bool result)) {
	bool result = vm_delete(vmid);
	callback(result);
}

static void vm_list_handler(uint32_t* ids, int size, void* context, void(*callback)(int size)) {
	size = vm_list(ids, size);
	callback(size);
}

static void status_get_handler(uint32_t vmid, void* context, void(*callback)(VMStatus status)) {
	VMStatus status = vm_status_get(vmid);
	callback(status);
}

static void status_set_handler(uint32_t vmid, VMStatus status, void* context, void(*callback)(bool result)) {
	void status_setted(bool result, void* context) {
		callback(result);
	}
	
	vm_status_set(vmid, status, status_setted, NULL);
}

static void storage_download_handler(uint32_t vmid, uint32_t offset, void** buf, int32_t size, void* context, void(*callback)(int32_t size)) {
	if(size < 0) {
		callback(size);
	} else {
		size = vm_storage_read(vmid, buf, offset, size);
		callback(size);
	}
}

static void storage_upload_handler(uint32_t vmid, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(int32_t size)) {
	if(size < 0) {
		callback(size);
	} else {
		if(offset == 0) {
			ssize_t len;
			if((len = vm_storage_clear(vmid)) < 0) {
				callback(len);
				return;
			}
		}
		
		size = vm_storage_write(vmid, buf, offset, size);
		callback(size);
	}
}

static void stdio_handler(uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(uint16_t size)) {
	ssize_t len = vm_stdio(id, thread_id, fd, str, size);
	callback(len >= 0 ? len : 0);
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
		
		rpc_loop(rpc);
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
