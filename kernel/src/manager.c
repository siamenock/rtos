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
#include <util/event.h>
#undef BYTE_ORDER
#include <netif/etharp.h>
#include <arch/driver.h>
#include <control/rpc.h>
#include <net/interface.h>
#include "gmalloc.h"
#include "malloc.h"
#include "mp.h"
#include "shell.h"
#include "vm.h"
#include "stdio.h"
#include "driver/nicdev.h"

#include "manager.h"

Manager manager;
extern NIC* __nics[NIC_MAX_COUNT];
extern int __nic_count;

static struct tcp_pcb* manager_server;

// LwIP TCP callbacks
typedef struct {
	struct tcp_pcb*	pcb;
	List*		pbufs;
	int		poll_count;
} RPCData;

static err_t manager_poll(void* arg, struct tcp_pcb* pcb);

static void rpc_free(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;

	ListIterator iter;
	list_iterator_init(&iter, data->pbufs);
	while(list_iterator_has_next(&iter)) {
		struct pbuf* pbuf = list_iterator_next(&iter);
		list_iterator_remove(&iter);
		pbuf_free(pbuf);
	}
	list_destroy(data->pbufs);
	list_remove_data(manager.actives, rpc);
	free(rpc);

	list_remove_data(manager.clients, data->pcb);
}

static void manager_close(struct tcp_pcb* pcb, RPC* rpc, bool is_force) {
	printf("Close connection: %p\n", pcb);
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_err(pcb, NULL);
	tcp_poll(pcb, NULL, 0);

	if(rpc) {
		rpc_free(rpc);
	} else {
		list_remove_data(manager.clients, pcb);
	}

	if(is_force) {
		tcp_abort(pcb);
	} else if(tcp_close(pcb) != ERR_OK) {
		printf("Cannot close pcb: %p %p\n", pcb, rpc);
		//tcp_poll(pcb, manager_poll, 2);
	}
}

static err_t manager_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
	RPC* rpc = arg;

	if(p == NULL) {	// Remote host closed the connection
		manager_close(pcb, rpc, false);
	} else if(err != ERR_OK) {
		pbuf_free(p);
	} else {
		RPCData* data = (RPCData*)rpc->data;
		list_add(data->pbufs, p);

		rpc_loop(rpc);
		tcp_recved(pcb, p->len);
		if(rpc_is_active(rpc)) {
			if(list_index_of(manager.actives, rpc, NULL) < 0) {
				list_add(manager.actives, rpc);
			}
		}
	}

	return ERR_OK;
}

static void manager_err(void* arg, err_t err) {
	RPC* rpc = arg;
	if(rpc != NULL) {
		RPCData* data = (RPCData*)rpc->data;
		printf("Error closed: %d %p\n", err, data->pcb);
		rpc_free(rpc);
	}
}

static err_t manager_poll(void* arg, struct tcp_pcb* pcb) {
	RPC* rpc = arg;

	if(rpc == NULL) {
		manager_close(pcb, NULL, true);
	} else {
		RPCData* data = (RPCData*)rpc->data;
		data->poll_count++;

		if(rpc->ver == 0 && data->poll_count++ >= 4) {	// 2 seconds
			printf("Close connection: not receiving hello in 2 secs.\n");
			manager_close(pcb, rpc, false);
		}
	}

	return ERR_OK;
}

// Handlers
static void vm_create_handler(RPC* rpc, VMSpec* vm_spec, void* context, void(*callback)(RPC* rpc, uint32_t id)) {
	uint32_t id = vm_create(vm_spec);
	callback(rpc, id);
}

static void vm_get_handler(RPC* rpc, uint32_t vmid, void* context, void(*callback)(RPC* rpc, VMSpec* vm)) {
	// TODO: Implement it
	callback(rpc, NULL);
}

static void vm_set_handler(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, bool result)) {
	// TODO: Implement it
	callback(rpc, false);
}

static void vm_destroy_handler(RPC* rpc, uint32_t vmid, void* context, void(*callback)(RPC* rpc, bool result)) {
	bool result = vm_destroy(vmid);
	callback(rpc, result);
}

static void vm_list_handler(RPC* rpc, int size, void* context, void(*callback)(RPC* rpc, uint32_t* ids, int size)) {
	uint32_t ids[16];
	size = vm_list(ids, size <= 16 ? size : 16);
	callback(rpc, ids, size);
}

static void status_get_handler(RPC* rpc, uint32_t vmid, void* context, void(*callback)(RPC* rpc, VMStatus status)) {
	VMStatus status = vm_status_get(vmid);
	callback(rpc, status);
}

typedef struct {
	RPC* rpc;
	struct tcp_pcb*	pcb;
	void(*callback)(RPC* rpc, bool result);
} Data;

static void status_setted(bool result, void* context) {
	Data* data = context;

	if(list_index_of(manager.clients, data->pcb, NULL) >= 0) {
		data->callback(data->rpc, result);
	}
	free(data);
}

static void status_set_handler(RPC* rpc, uint32_t vmid, VMStatus status, void* context, void(*callback)(RPC* rpc, bool result)) {
	Data* data = malloc(sizeof(Data));
	data->rpc = rpc;
	data->pcb = context;
	data->callback = callback;

	vm_status_set(vmid, status, status_setted, data);
}

static void storage_download_handler(RPC* rpc, uint32_t vmid, uint64_t download_size, uint32_t offset, int32_t size, void* context, void(*callback)(RPC* rpc, void* buf, int32_t size)) {
	if(size < 0) {
		callback(rpc, NULL, size);
	} else {
		void* buf;
		if(download_size)
			size = vm_storage_read(vmid, &buf, offset, (offset + size > download_size) ? (download_size - offset) : (uint32_t)size);
		else
			size = vm_storage_read(vmid, &buf, offset, (uint32_t)size);

		callback(rpc, buf, size);
	}
}

static void storage_upload_handler(RPC* rpc, uint32_t vmid, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(RPC* rpc, int32_t size)) {
	static int total_size = 0;
	if(size < 0) {
		callback(rpc, size);
	} else {
		if(offset == 0) {
			ssize_t len;
			if((len = vm_storage_clear(vmid)) < 0) {
				callback(rpc, len);
				return;
			}
		}

		if(size < 0) {
			printf(". Aborted: %d\n", size);
			callback(rpc, size);
		} else {
			size = vm_storage_write(vmid, buf, offset, size);
			callback(rpc, size);

			if(size > 0) {
				printf(".");
				total_size += size;
			} else if(size == 0) {
				printf(". Done. Total size: %d\n", total_size);
				total_size = 0;
			} else if(size < 0) {
				printf(". Error: %d\n", size);
				total_size = 0;
			}
		}
	}
}

static void stdio_handler(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size)) {
	ssize_t len = vm_stdio(id, thread_id, fd, str, size);
	callback(rpc, len >= 0 ? len : 0);
}

static void storage_md5_handler(RPC* rpc, uint32_t id, uint64_t size, void* context, void(*callback)(RPC* rpc, bool result, uint32_t md5[])) {
	uint32_t md5sum[4];
	bool ret = vm_storage_md5(id, size, md5sum);

	callback(rpc, ret, md5sum);
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

	if(tcp_output(data->pcb) != ERR_OK) {
		errno = -2;
		return -1;
	}

	return len;
}

static void pcb_close(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	manager_close(data->pcb, rpc, false);
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
	rpc_vm_destroy_handler(rpc, vm_destroy_handler, NULL);
	rpc_vm_list_handler(rpc, vm_list_handler, NULL);
	rpc_status_get_handler(rpc, status_get_handler, NULL);
	rpc_status_set_handler(rpc, status_set_handler, pcb);
	rpc_storage_download_handler(rpc, storage_download_handler, NULL);
	rpc_storage_upload_handler(rpc, storage_upload_handler, NULL);
	rpc_stdio_handler(rpc, stdio_handler, NULL);
	rpc_storage_md5_handler(rpc, storage_md5_handler, NULL);

	RPCData* data = (RPCData*)rpc->data;
	data->pcb = pcb;
	data->pbufs = list_create(NULL);

	tcp_arg(pcb, rpc);
	tcp_recv(pcb, manager_recv);
	tcp_err(pcb, manager_err);
	tcp_poll(pcb, manager_poll, 2);

	list_add(manager.clients, pcb);

	return ERR_OK;
}

// RPC Manager
static void stdio_callback(uint32_t vmid, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
	ListIterator iter;
	list_iterator_init(&iter, manager.clients);
	size_t len0, len1, len2;
	bool wrapped;

	if(*head <= *tail) {
		wrapped = false;

		len0 = *tail - *head;
	} else {
		wrapped = true;

		len1 = size - *head;
		len2 = *tail;
	}

	while(list_iterator_has_next(&iter)) {
		struct tcp_pcb* pcb = list_iterator_next(&iter);
		RPC* rpc = pcb->callback_arg;

		if(wrapped) {
			rpc_stdio(rpc, vmid, thread_id, fd, buffer + *head, len1, NULL, NULL);
			rpc_stdio(rpc, vmid, thread_id, fd, buffer, len2, NULL, NULL);

		} else {
			rpc_stdio(rpc, vmid, thread_id, fd, buffer + *head, len0, NULL, NULL);
		}

		rpc_loop(rpc);
	}

	if(wrapped) {
		*head = (*head + len1 + len2) % size;
	} else {
		*head += len0;
	}
}

static bool manager_loop(void* context) {
	//netif open
	//netif close
	ListIterator iter;
	list_iterator_init(&iter, manager.netifs);
	while(list_iterator_has_next(&iter)) {
		struct netif* netif = list_iterator_next(&iter);
		NIC* nic = ((struct netif_private*)netif->state)->nic;
		if(!nic_has_rx(nic))
			continue;
		
		Packet* packet = nic_rx(nic);
		if(packet) {
			lwip_nic_poll(netif, packet);
		}
	}

	if(!list_is_empty(manager.actives)) {
		ListIterator iter;
		list_iterator_init(&iter, manager.actives);
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

static bool manager_timer(void* context) {
	lwip_nic_timer();

	return true;
}

//TODO gabage collection
VNIC* manager_create_vnic(char* name) {
	if(manager.vnic_count > NIC_MAX_COUNT)
		return NULL;

	NICDevice* nicdev = nicdev_get(name);
	if(!nicdev) {
		printf("\tCannot create manager\n");
		return false;
	}

	uint64_t attrs[] = {
		//VNIC_MAC, nicdev->mac,
		VNIC_MAC, nicdev->mac,
		VNIC_DEV, (uint64_t)nicdev->name,
		VNIC_BUDGET, 32,
		VNIC_POOL_SIZE, 0x200000,
		VNIC_RX_BANDWIDTH, 1000000000L,
		VNIC_TX_BANDWIDTH, 1000000000L,
		VNIC_RX_QUEUE_SIZE, 1024,
		VNIC_TX_QUEUE_SIZE, 1024,
		VNIC_PADDING_HEAD, 32,
		VNIC_PADDING_TAIL, 32,
		VNIC_SLOW_RX_QUEUE_SIZE, 1024,
		VNIC_SLOW_TX_QUEUE_SIZE, 1024,
		VNIC_NONE
	};

	VNIC* vnic = gmalloc(sizeof(VNIC));
	if(!vnic)
		return false;
	memset(vnic, 0, sizeof(VNIC));

	vnic->id = vnic_alloc_id();
	vnic->nic_size = 0x200000;
	vnic->nic = bmalloc(1);
	if(!vnic->nic)
		return false;
	memset(vnic->nic, 0, 0x200000);

	if(!vnic_init(vnic, attrs))
		return false;

	int vnic_id = nicdev_register_vnic(nicdev, vnic);
	if(vnic_id < 0)
		return false;

	for(int i = 0; i < NIC_MAX_COUNT; i++) {
		if(manager.vnics[i])
			continue;

		manager.vnics[i] = vnic;
		manager.vnic_count++;
		__nics[i] = vnic->nic;
		__nic_count++;
		break;
	}

	return vnic;
}

bool manager_destroy_vnic(VNIC* vnic) {
	for(int i = 0; i < NIC_MAX_COUNT; i++) {
		if(manager.vnics[i] != vnic)
			continue;

		manager.vnics[i] = NULL;
		manager.vnic_count--;
		__nics[i] = NULL;
		__nic_count++;
		break;
	}

	NICDevice* nic_dev = nicdev_get(vnic->parent);
	if(!nic_dev)
		return false;

	if(!nicdev_unregister_vnic(nic_dev, vnic->id))
		return false;

	bfree(vnic->nic);
	gfree(vnic);

	return true;
}

VNIC* manager_get_vnic(char* name) {
	if(strncmp(name, "veth", 4))
		return NULL;

	char* index;
	strtok_r(name + 4, ".", &index);
	if(!is_uint8(index))
		return NULL;

	return manager.vnics[parse_uint8(index)];
}

static Packet* rx_process(Packet* packet) {
	return packet;
}

static Packet* tx_process(Packet* packet) {
	return packet;
}

struct netif* manager_create_netif(NIC* nic, uint32_t ip, uint32_t netmask, uint32_t gateway, bool is_default) {
	IPv4Interface* interface = interface_alloc(nic,
			ip, netmask, gateway, is_default);
	if(!interface) {
		return false;
	}

	struct netif* netif = malloc(sizeof(struct netif));
	struct netif_private* private = malloc(sizeof(struct netif_private));

	private->nic = nic;
	private->rx_process = rx_process;
	private->tx_process = tx_process;
	
	struct ip_addr ip2, netmask2, gateway2;
	IP4_ADDR(&ip2, (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip >> 0) & 0xff);
	IP4_ADDR(&netmask2, (netmask >> 24) & 0xff, (netmask >> 16) & 0xff, (netmask >> 8) & 0xff, (netmask >> 0) & 0xff);
	IP4_ADDR(&gateway2, (gateway >> 24) & 0xff, (gateway >> 16) & 0xff, (gateway >> 8) & 0xff, (gateway >> 0) & 0xff);
	
	netif_add(netif, &ip2, &netmask2, &gateway2, private, lwip_driver_init, ethernet_input);

	if(is_default)
		netif_set_default(netif);

	netif_set_up(netif);
	list_add(manager.netifs, netif);

	return netif;
}

bool manager_destroy_netif(struct netif* netif) {
	return false;
}

struct netif* manager_get_netif(uint32_t ip) {
	return NULL;
}

inline void manager_netif_set_ip(struct netif* netif, uint32_t ip) {
	struct ip_addr ip2;
	IP4_ADDR(&ip2, (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip >> 0) & 0xff);
	netif_set_ipaddr(netif, &ip2);
}

inline void manager_netif_set_gateway(struct netif* netif, uint32_t gateway) {
	struct ip_addr gateway2;
	IP4_ADDR(&gateway2, (gateway >> 24) & 0xff, (gateway >> 16) & 0xff, (gateway >> 8) & 0xff, (gateway >> 0) & 0xff);
	netif_set_gateway(netif, &gateway2);
}

inline void manager_netif_set_netmask(struct netif* netif, uint32_t netmask) {
	struct ip_addr netmask2;
	IP4_ADDR(&netmask2, (netmask >> 24) & 0xff, (netmask >> 16) & 0xff, (netmask >> 8) & 0xff, (netmask >> 0) & 0xff);
	netif_set_netmask(netif, &netmask2);
}

struct tcp_pcb* manager_netif_server_open(struct netif* netif, uint16_t port) {
	struct ip_addr ip_addr = netif->ip_addr;

	struct tcp_pcb* tcp_pcb = tcp_new();
	err_t err = tcp_bind(tcp_pcb, &ip_addr, port);
	if(err != ERR_OK) {
		printf("ERROR: Manager cannot bind TCP session: %d\n", err);

		return false;
	}

	tcp_pcb = tcp_listen(tcp_pcb);
	tcp_arg(tcp_pcb, tcp_pcb);

	printf("Manager started: %d.%d.%d.%d:%d\n", ip_addr.addr & 0xff, (ip_addr.addr >> 8) & 0xff,
		(ip_addr.addr >> 16) & 0xff, (ip_addr.addr >> 24) & 0xff, port);

	tcp_accept(tcp_pcb, manager_accept);

	return tcp_pcb;
}

bool manager_netif_server_close(struct tcp_pcb* tcp_pcb) {
	ListIterator iter;
	list_iterator_init(&iter, manager.clients);
	while(list_iterator_has_next(&iter)) {
		struct tcp_pcb* pcb = list_iterator_next(&iter);
		RPC* rpc = pcb->callback_arg;
		manager_close(pcb, rpc, true);
	}

	if(tcp_close(tcp_pcb) != ERR_OK) {
		printf("Cannot close manager server: %p", tcp_pcb);

		return false;
	}

	return true;
}

//TODO command
int manager_init() {
	lwip_init();
	manager.netifs = list_create(NULL);
	if(!manager.netifs)
		return -1;

	manager.servers = list_create(NULL);
	if(!manager.servers)
		return -1;

	manager.clients = list_create(NULL);
	if(!manager.clients)
		return -1;

	manager.actives = list_create(NULL);
	if(!manager.actives)
		return -1;

	VNIC* vnic = manager_create_vnic(MANAGER_DEFAULT_NICDEV);
	if(!vnic)
		return -2;

	//TODO: add manager init script
	struct netif* netif = manager_create_netif(vnic->nic, MANAGER_DEFAULT_IP, MANAGER_DEFAULT_NETMASK, MANAGER_DEFAULT_GW, true);
	if(!netif)
		return -3;

	struct tcp_pcb* pcb = manager_netif_server_open(netif, MANAGER_DEFAULT_PORT);
	if(!pcb)
		return -4;

	manager_server = pcb;

	if(!event_idle_add(manager_loop, NULL))
		return -5;

	if(!event_timer_add(manager_timer, NULL, 100000, 100000))
		return -6;

	vm_stdio_handler(stdio_callback);

	return 0;
}
