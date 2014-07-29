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
#include <net/md5.h>
#include <util/map.h>
#include <util/ring.h>
#include <util/event.h>
#include "ni.h"
#include "malloc.h"
#include "gmalloc.h"
#include "mp.h"
#include "loader.h"
#include "icc.h"
#include "cpu.h"
#include "vm.h"
#include "shell.h"
#include "stdio.h"
#include "rpc/rpc_manager.h"
#include "rpc/rpc_callback.h"

#include "manager.h"

#define DEFAULT_MANAGER_IP	0xc0a864fe	// 192.168.100.254
#define DEFAULT_MANAGER_GW	0xc0a864c8	// 192.168.100.200
#define DEFAULT_MANAGER_NM	0xffffff00	// 255.255.255.0

static uint64_t	last_vmid = 1;
static Map*	vms;

NI*	manager_ni;

// Core status
typedef struct {
	int		status;	// VM_STATUS_XXX
	int		error_code;
	
	VM*		vm;
	
	char*			stdin;
	volatile size_t*	stdin_head;
	volatile size_t*	stdin_tail;
	size_t			stdin_size;
	
	char*			stdout;
	volatile size_t*	stdout_head;
	volatile size_t*	stdout_tail;
	size_t			stdout_size;
	
	char*			stderr;
	volatile size_t*	stderr_head;
	volatile size_t*	stderr_tail;
	size_t			stderr_size;
} Core;

typedef struct {
	uint32_t	addr;
	uint16_t	port;
	CLIENT*		client;
	uint64_t	time;
} Callback;

static Core cores[MP_MAX_CORE_COUNT];
static List* callbacks;

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

static void icc_started(ICC_Message* msg) {
	Core* core = &cores[msg->core_id];
	VM* vm = core->vm;
	
	if(msg->result == 0) {
		core->error_code = 0;
		core->status = VM_STATUS_STARTED;
		core->stdin = msg->data.started.stdin;
		core->stdin_head = msg->data.started.stdin_head;
		core->stdin_tail = msg->data.started.stdin_tail;
		core->stdin_size = msg->data.started.stdin_size;
		core->stdout = msg->data.started.stdout;
		core->stdout_head = msg->data.started.stdout_head;
		core->stdout_tail = msg->data.started.stdout_tail;
		core->stdout_size = msg->data.started.stdout_size;
		core->stderr = msg->data.started.stderr;
		core->stderr_head = msg->data.started.stderr_head;
		core->stderr_tail = msg->data.started.stderr_tail;
		core->stderr_size = msg->data.started.stderr_size;
		
		core->status = VM_STATUS_STARTED;
		
		printf("Execution succeed on core[%d].\n", msg->core_id);
		
		msg->status = ICC_STATUS_DONE;
	} else {
		core->error_code = msg->result;
		
		core->status = VM_STATUS_STOPPED;
		
		printf("Execution FAILED on core[%d]: Error code 0x%x.\n", msg->core_id, msg->result);
		
		msg->status = ICC_STATUS_DONE;
	}
	
	int error_code = 0;
	for(int i = 0; i < vm->core_size; i++) {
		core = &cores[vm->cores[i]];
		
		if(core->status == VM_STATUS_STARTED) {
			continue;
		}
		
		if(core->error_code != 0) {
			error_code = core->error_code;
			continue;
		}
		
		return;
	}
	
	vm->status = error_code == 0 ? VM_STATUS_STARTED : VM_STATUS_STOPPED;
	
	event_trigger_fire(EVENT_VM_STARTED, vm, NULL, NULL);
	
	if(error_code != 0) {
		for(int i = 0; i < vm->core_size; i++) {
			if(cores[vm->cores[i]].status == VM_STATUS_STARTED) {
				ICC_Message* msg = icc_sending(ICC_TYPE_STOP, vm->cores[i]);
				icc_send(msg);
			}
		}
	}
	
	if(error_code == 0) {
		printf("VM started on cores[");
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size) {
				printf(", ");
			}
		}
		printf("]\n");
	} else {
		printf("VM started with error(s) on cores[");
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			error_code = cores[vm->cores[i]].error_code;
			if(error_code != 0)
				printf("(0x%x)", error_code);
			
			if(i + 1 < vm->core_size) {
				printf(", ");
			}
		}
		printf("]\n");
	}
}

static void icc_paused(ICC_Message* msg) {
	VM* vm = cores[msg->core_id].vm;
	
	cores[msg->core_id].status = VM_STATUS_PAUSED;
	
	printf("Execution paused on core[%d].\n", msg->core_id);
	
	msg->status = ICC_STATUS_DONE;
	
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status != VM_STATUS_PAUSED)
			return;
	}
	
	vm->status = VM_STATUS_PAUSED;
	
	event_trigger_fire(EVENT_VM_PAUSED, vm, NULL, NULL);
	
	printf("VM paused on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

static void icc_resumed(ICC_Message* msg) {
	VM* vm = cores[msg->core_id].vm;
	
	cores[msg->core_id].status = VM_STATUS_STARTED;
	
	printf("Execution resumed on core[%d].\n", msg->core_id);
	
	msg->status = ICC_STATUS_DONE;
	
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status != VM_STATUS_STARTED)
			return;
	}
	
	vm->status = VM_STATUS_STARTED;
	
	event_trigger_fire(EVENT_VM_RESUMED, vm, NULL, NULL);
	
	printf("VM resumed on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

static void icc_stopped(ICC_Message* msg) {
	VM* vm = cores[msg->core_id].vm;
	
	cores[msg->core_id].status = VM_STATUS_STOPPED;
	
	printf("Execution completed on core[%d].\n", msg->core_id);
	
	msg->status = ICC_STATUS_DONE;
	
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status != VM_STATUS_STOPPED)
			return;
	}
	
	vm->status = VM_STATUS_STOPPED;
	
	event_trigger_fire(EVENT_VM_STOPPED, vm, NULL, NULL);
	
	printf("VM stopped on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

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
	
	// Standard I/O/E processing
	int get_thread_id(VM* vm, int core) {
		for(int i = 0; i < vm->core_size; i++) {
			if(vm->cores[i] == core)
				return i;
		}
		
		return -1;
	}
	
	void output(uint64_t vmid, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
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
			if(!rpc_call_async(ni, cb->client, STDIO, (xdrproc_t)xdr_RPC_Message, (char*)&msg))
				printf("Cannot send Std I/O message: %d\n", errno);
		}
		free(buf);
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
	
	static int core_index;
	
	int core_count = mp_core_count();
	for(int i = 0; i < core_count; i++) {
		core_index = (core_index + 1) % MP_MAX_CORE_COUNT;
		
		Core* core = &cores[core_index];
		
		if(core->status != VM_STATUS_STOPPED) {
			int thread_id = -1;
			
			if(core->stdout != NULL && *core->stdout_head != *core->stdout_tail) {
				thread_id = get_thread_id(core->vm, core_index);
				
				output(core->vm->id, thread_id, 1, core->stdout, core->stdout_head, core->stdout_tail, core->stdout_size);
			}
			
			if(core->stderr != NULL && *core->stderr_head != *core->stderr_tail) {
				if(thread_id == -1)
					thread_id = get_thread_id(core->vm, core_index);
				
				output(core->vm->id, thread_id, 2, core->stderr, core->stderr_head, core->stderr_tail, core->stderr_size);
			}
			
			break;
		}
	}
	
	for(int i = 1; i < core_count; i++) {
		char* buffer = (char*)MP_CORE(__stdout, i);
		volatile size_t* head = (size_t*)MP_CORE(&__stdout_head, i);
		volatile size_t* tail = (size_t*)MP_CORE(&__stdout_tail, i);
		size_t size = *(size_t*)MP_CORE(&__stdout_size, i);
		
		while(*head != *tail) {
			output(0, i, 1, buffer, head, tail, size);
		}
		
		buffer = (char*)MP_CORE(__stderr, i);
		head = (size_t*)MP_CORE(&__stderr_head, i);
		tail = (size_t*)MP_CORE(&__stderr_tail, i);
		size = *(size_t*)MP_CORE(&__stderr_size, i);
		
		while(*head != *tail) {
			output(0, i, 2, buffer, head, tail, size);
		}
	}
	
	return true;
}

static bool vm_destroy(VM* vm, int core) {
	bool is_destroy = true;
	
	if(core == -1) {
		for(uint32_t i = 0; i < vm->core_size; i++) {
			if(vm->cores[i]) {
				cores[vm->cores[i]].status = VM_STATUS_STOPPED;
			}
		}
	} else {
		for(uint32_t i = 0; i < vm->core_size; i++) {
			if(vm->cores[i] == core) {
				cores[vm->cores[i]].status = VM_STATUS_STOPPED;
			}
			
			if(!cores[vm->cores[i]].status == VM_STATUS_STOPPED)
				is_destroy = false;
		}
	}
	
	if(is_destroy) {
		if(vm->memory.blocks) {
			for(int i = 0; i < vm->memory.count; i++) {
				if(vm->memory.blocks[i]) {
					bfree(vm->memory.blocks[i]);
				}
			}
			
			gfree(vm->memory.blocks);
		}
		
		if(vm->storage.blocks) {
			for(int i = 0; i < vm->storage.count; i++) {
				if(vm->storage.blocks[i]) {
					bfree(vm->storage.blocks[i]);
				}
			}
			
			gfree(vm->storage.blocks);
		}
		
		if(vm->nics) {
			for(uint16_t i = 0; i < vm->nic_size; i++) {
				if(vm->nics[i])
					ni_destroy(vm->nics[i]);
			}
			
			gfree(vm->nics);
		}
		
		if(vm->argv) {
			gfree(vm->argv);
		}
		
		gfree(vm);
	}
	
	return is_destroy;
}

void * manager_null_1_svc(struct svc_req *rqstp) {
	static void* result;
	return (void*)&result;
}

u_quad_t * vm_create_1_svc(RPC_VM rpc_vm,  struct svc_req *rqstp) {
	static u_quad_t result;
	result = 0;
	
	VM* vm = gmalloc(sizeof(VM));
	bzero(vm, sizeof(VM));
	
	// Allocate args
	int argc = 0;
	char* argv[1024];
	char* args = rpc_vm.args.args_val;
	int len = rpc_vm.args.args_len;
	for(int i = 0; i < len; ) {
		argv[argc++] = args + i;
		i += strlen(args + i) +  1;
	}
	
	vm->argv = gmalloc(sizeof(char*) * argc + rpc_vm.args.args_len);
	vm->argc = argc;
	memcpy(vm->argv, argv, sizeof(char*) * argc);
	memcpy(vm->argv + sizeof(char*) * argc, rpc_vm.args.args_val, rpc_vm.args.args_len);
	
	// Allocate core
	vm->core_size = rpc_vm.core_num;
	
	int j = 0;
	int core_count = mp_core_count();
	for(int i = 0; i < core_count; i++) {
		if(cores[i].status == VM_STATUS_STOPPED) {
			vm->cores[j++] = i;
			cores[i].status = VM_STATUS_PAUSED;
			cores[i].vm = vm;
			
			if(j >= vm->core_size)
				break;
		}
	}
	
	if(j < vm->core_size) {
		printf("Manager: Not enough core to allocate.\n");
		vm_destroy(vm, -1);
		return &result;
	}
	
	// Allocate memory
	uint64_t memory_size = rpc_vm.memory_size;
	memory_size = (memory_size + (VM_MEMORY_SIZE_ALIGN - 1)) & ~(VM_MEMORY_SIZE_ALIGN - 1);
	vm->memory.count = memory_size / VM_MEMORY_SIZE_ALIGN;
	vm->memory.blocks = gmalloc(vm->memory.count * sizeof(void*));
	bzero(vm->memory.blocks, vm->memory.count * sizeof(void*));
	for(int i = 0; i < vm->memory.count; i++) {
		vm->memory.blocks[i] = bmalloc();
		if(!vm->memory.blocks[i]) {
			printf("Manager: Not enough memory to allocate.\n");
			vm_destroy(vm, -1);
			return &result;
		}
	}
	
	// Allocate storage
	uint64_t storage_size = rpc_vm.storage_size;
	storage_size = (storage_size + (VM_STORAGE_SIZE_ALIGN - 1)) & ~(VM_STORAGE_SIZE_ALIGN - 1);
	vm->storage.count = storage_size / VM_STORAGE_SIZE_ALIGN;
	vm->storage.blocks = gmalloc(vm->storage.count * sizeof(void*));
	bzero(vm->storage.blocks, vm->storage.count * sizeof(void*));
	for(int i = 0; i < vm->storage.count; i++) {
		vm->storage.blocks[i] = bmalloc();
		if(!vm->storage.blocks[i]) {
			printf("Manager: Not enough storage to allocate.\n");
			vm_destroy(vm, -1);
			return &result;
		}
	}
	
	// Allocate NICs
	RPC_NIC* rpc_nic = rpc_vm.nics.nics_val;
	vm->nic_size = rpc_vm.nics.nics_len;
	vm->nics = gmalloc(sizeof(NI) * vm->nic_size);
	bzero(vm->nics, sizeof(NI) * vm->nic_size);
	
	for(int i = 0; i < vm->nic_size; i++) {
		uint64_t mac = rpc_nic[i].mac;
		if(mac == 0) {
			do {
				mac = cpu_tsc() & 0x0fffffffffffL;
				mac |= 0x02L << 40;	// Locally administrered
			} while(ni_contains(mac));
		} else if(ni_contains(mac)) {
			printf("Manager: The mac address already in use: %012x.\n", mac);
			vm_destroy(vm, -1);
			return &result;
		}
		
		uint64_t attrs[] = { 
			NI_MAC, mac,
			NI_INPUT_BUFFER_SIZE, rpc_nic[i].input_buffer_size,
			NI_OUTPUT_BUFFER_SIZE, rpc_nic[i].output_buffer_size,
			NI_INPUT_BANDWIDTH, rpc_nic[i].input_bandwidth,
			NI_OUTPUT_BANDWIDTH, rpc_nic[i].output_bandwidth,
			NI_POOL_SIZE, rpc_nic[i].pool_size,
			NI_INPUT_ACCEPT_ALL, 1,
			NI_OUTPUT_ACCEPT_ALL, 1,
			NI_INPUT_FUNC, 0,
			NI_NONE
		};
		
		vm->nics[i] = ni_create(attrs);
		if(!vm->nics[i]) {
			printf("Manager: Not enough NIC to allocate: errno=%d.\n", errno);
			vm_destroy(vm, -1);
			return &result;
		}
	}
	
	// Allocate vmid
	uint64_t vmid;
	while(true) {
		vmid = last_vmid++;
		
		if(vmid != 0 && !map_contains(vms, (void*)vmid)) {
			vm->id = vmid;
			map_put(vms, (void*)vmid, vm);
			break;
		}
	}
	result = vmid;
	
	// Dump info
	printf("Manager: VM[%d] created(cores [", vmid);
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size)
			printf(", ");
	}
	printf("], %dMBs memory, %dMBs storage, NICs: %d\n",
		vm->memory.count * VM_MEMORY_SIZE_ALIGN / 0x100000,
		vm->storage.count * VM_STORAGE_SIZE_ALIGN / 0x100000, vm->nic_size);
	
	for(int i = 0; i < vm->nic_size; i++) {
		NI* ni = vm->nics[i];
		printf("\t");
		for(int j = 5; j >= 0; j--) {
			printf("%02x", (ni->mac >> (j * 8)) & 0xff);
			if(j - 1 >= 0)
				printf(":");
			else
				printf(" ");
		}
		printf("%dMbps/%d, %dMbps/%d, %dMBs\n", ni->input_bandwidth / 1000000, fifo_capacity(ni->ni->input_buffer) + 1, ni->output_bandwidth / 1000000, fifo_capacity(ni->ni->output_buffer) + 1, list_size(ni->pools) * 2);
	}
	
	printf("\targs(%d): ", vm->argc);
	for(int i = 0; i < vm->argc; i++) {
		char* ch = strchr(vm->argv[i], ' ');
		if(ch == NULL)
			printf("%s ", vm->argv[i]);
		else
			printf("\"%s\" ", vm->argv[i]);
	}
	printf("\n");
	
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
	result = FALSE;
	
	VM* vm = map_get(vms, (void*)vmid);
	if(!vm)
		return &result;
	
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status == VM_STATUS_STARTED) {
			return &result;
		}
	}
	
	printf("Manager: Delete vm[%d] on cores [", vmid);
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size)
			printf(", ");
	}
	printf("]\n");
	
	map_remove(vms, (void*)vmid);
	vm_destroy(vm, -1);
	
	result = TRUE;
	return &result;
}

RPC_VMList * vm_list_1_svc(struct svc_req *rqstp) {
	static RPC_VMList result;
	static u_quad_t vmids[RPC_MAX_VM_COUNT];
	
	result.RPC_VMList_len = 0;
	result.RPC_VMList_val = NULL;
	
	MapIterator iter;
	map_iterator_init(&iter, vms);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		vmids[result.RPC_VMList_len++] = (uint64_t)entry->key;
	}
	result.RPC_VMList_val = vmids;
	
	return &result;
}

bool_t * status_set_1_svc(u_quad_t vmid, RPC_VMStatus status, struct svc_req *rqstp) {
	static bool_t result = FALSE;
	
	VM* vm = map_get(vms, (void*)vmid);
	if(!vm) {
		return &result;
	}
	
	bool callback(uint64_t event_type, void* event, void* context) {
		SVCXPRT* transp = context;
		VM* vm = event;
		
		bool_t result;
		switch(event_type) {
			case EVENT_VM_STARTED:
				result = vm->status == VM_STATUS_STARTED ? TRUE : FALSE;
				break;
			case EVENT_VM_PAUSED:
				result = vm->status == VM_STATUS_PAUSED ? TRUE : FALSE;
				break;
			case EVENT_VM_RESUMED:
				result = vm->status == VM_STATUS_STARTED ? TRUE : FALSE;
				break;
			case EVENT_VM_STOPPED:
				result = vm->status == VM_STATUS_STOPPED ? TRUE : FALSE;
				break;
			default:
				svcerr_systemerr(transp);
				free(transp);
				return false;
		}
		
		if(!svc_sendreply(transp, (xdrproc_t)xdr_bool, (caddr_t)&result)) {
			svcerr_systemerr(transp);
		}
		free(transp);
		
		return false;
	}
	
	void start(uint64_t vmid, VM* vm) {
		printf("Manager: VM[%d] starts on cores [", vmid);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("]\n");
		
		SVCXPRT* transp = malloc(sizeof(SVCXPRT));
		memcpy(transp, rqstp->rq_xprt, sizeof(SVCXPRT));
		event_trigger_add(EVENT_VM_STARTED, callback, transp);
		
		for(int i = 0; i < vm->core_size; i++) {
			cores[vm->cores[i]].error_code = 0;
			ICC_Message* msg = icc_sending(ICC_TYPE_START, vm->cores[i]);
			msg->data.start.vm = vm;
			icc_send(msg);
		}
	}
	
	void pause(uint64_t vmid, VM* vm) {
		printf("Manager: VM[%d] pauses on cores [", vmid);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("]\n");
		
		SVCXPRT* transp = malloc(sizeof(SVCXPRT));
		memcpy(transp, rqstp->rq_xprt, sizeof(SVCXPRT));
		event_trigger_add(EVENT_VM_PAUSED, callback, transp);
		
		for(int i = 0; i < vm->core_size; i++) {
			ICC_Message* msg = icc_sending(ICC_TYPE_PAUSE, vm->cores[i]);
			icc_send(msg);
		}
	}
	
	void resume(uint64_t vmid, VM* vm) {
		printf("Manager: VM[%d] resume on cores [", vmid);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("]\n");
		
		SVCXPRT* transp = malloc(sizeof(SVCXPRT));
		memcpy(transp, rqstp->rq_xprt, sizeof(SVCXPRT));
		event_trigger_add(EVENT_VM_RESUMED, callback, transp);
		
		for(int i = 0; i < vm->core_size; i++) {
			ICC_Message* msg = icc_sending(ICC_TYPE_RESUME, vm->cores[i]);
			icc_send(msg);
		}
	}
	
	void stop(uint64_t vmid, VM* vm) {
		printf("Manager: VM[%d] stops on cores [", vmid);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("]\n");
		
		SVCXPRT* transp = malloc(sizeof(SVCXPRT));
		memcpy(transp, rqstp->rq_xprt, sizeof(SVCXPRT));
		event_trigger_add(EVENT_VM_STOPPED, callback, transp);
		
		for(int i = 0; i < vm->core_size; i++) {
			ICC_Message* msg = icc_sending(ICC_TYPE_STOP, vm->cores[i]);
			icc_send(msg);
		}
	}
	
	if(status == RPC_START && vm->status == VM_STATUS_STOPPED) {
		start(vmid, vm);
		return NULL;
	} else if(status == RPC_PAUSE && vm->status == VM_STATUS_STARTED) {
		pause(vmid, vm);
		return NULL;
	} else if(status == RPC_RESUME && vm->status == VM_STATUS_PAUSED) {
		resume(vmid, vm);
		return NULL;
	} else if(status == RPC_STOP && (vm->status == VM_STATUS_PAUSED || vm->status == VM_STATUS_STARTED)) {
		stop(vmid, vm);
		return NULL;
	}
	
	return &result;	// Async call
}

RPC_VMStatus * status_get_1_svc(u_quad_t vmid,  struct svc_req *rqstp) {
	static RPC_VMStatus  result;
	result = RPC_NONE;
	
	VM* vm = map_get(vms, (void*)vmid);
	if(!vm) {
		return &result;
	}
	
	switch(vm->status) {
		case VM_STATUS_STOPPED:
			result = RPC_STOP;
			break;
		case VM_STATUS_PAUSED:
			result = RPC_PAUSE;
			break;
		case VM_STATUS_STARTED:
			result = RPC_START;
			break;
	}
	
	return &result;
}

RPC_Digest * storage_digest_1_svc(u_quad_t vmid, RPC_MessageDigestType type, u_quad_t size,  struct svc_req *rqstp) {
	static RPC_Digest  result;
	static uint32_t md5sum[4];
	
	result.RPC_Digest_len = 0;
	result.RPC_Digest_val = NULL;
	
	VM* vm = map_get(vms, (void*)vmid);
	if(!vm)
		return &result;
	
	md5_blocks(vm->storage.blocks, vm->storage.count, VM_STORAGE_SIZE_ALIGN, size, md5sum);
	
	result.RPC_Digest_len = 16;
	result.RPC_Digest_val = (char*)md5sum;
	
	return &result;
}

static uint32_t get_remote_addr(struct svc_req* rqstp) {
	Packet* packet = (Packet*)rqstp->rq_xprt->xp_p2;
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	
	return endian32(ip->source);
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

static int tftp_create(char* filename, int mode) { // 1: read, 2: write
	uint64_t vmid = (uint64_t)strtoll(filename, NULL, 0);
	if(map_contains(vms, (void*)vmid)) {
		printf("TFTP: %d [");
		return (int)vmid;	// TODO: Change vmid type to int
	} else {
		return -1;
	}
}

static int tftp_write(int fd, void* buf, uint32_t offset, int size) {
	VM* vm = map_get(vms, (void*)(uint64_t)fd);
	if(!vm)
		return -2;
	
	if((uint64_t)offset + size > (uint64_t)vm->storage.count * VM_STORAGE_SIZE_ALIGN) {
		return -1;
	}
	
	// TODO: Calc block index, fragmented block
	memcpy(vm->storage.blocks[0] + offset, buf, size);
	if(size == 512)
		printf(".");
	else
		printf("]\n");
	
	return size;
}

static int tftp_read(int fd, void* buf, uint32_t offset, int size) {
	VM* vm = map_get(vms, (void*)(uint64_t)fd);
	if(!vm)
		return -2;
	
	if((uint64_t)offset + size > (uint64_t)vm->storage.count * VM_STORAGE_SIZE_ALIGN) {
		return -1;
	}
	
	// TODO: Calc block index, fragmented block
	memcpy(buf, vm->storage.blocks[0] + offset, size);
	
	return size;
}

static TFTPCallback tftp_callback = { tftp_create, tftp_write, tftp_read };

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
	
	callbacks = list_create(malloc, free, NULL);
	
	SVCXPRT* rpc;
	if(!(rpc = svcudp_create(111)))
		printf("Cannot create service\n");
	
	void manager_1(struct svc_req *rqstp, register SVCXPRT *transp);
	if(!svc_register(rpc, MANAGER, MANAGER_APPLE, manager_1, IPPROTO_UDP))
		printf("Cannot register manager service\n");
	
	void callback_1(struct svc_req *rqstp, register SVCXPRT *transp);
	if(!svc_register(rpc, CALLBACK, CALLBACK_APPLE, callback_1, IPPROTO_UDP))
		printf("Cannot register callback service\n");
	
	vms = map_create(4, map_uint64_hash, map_uint64_equals, malloc, free, NULL);
	
	icc_register(ICC_TYPE_STARTED, icc_started);
	icc_register(ICC_TYPE_PAUSED, icc_paused);
	icc_register(ICC_TYPE_RESUMED, icc_resumed);
	icc_register(ICC_TYPE_STOPPED, icc_stopped);
	
	// Core 0 is occupied by manager
	cores[0].status = VM_STATUS_STARTED;
	
	event_idle_add((void*)manager_loop, manager_ni->ni);
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
