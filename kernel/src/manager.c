#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <net/md5.h>
#include <util/map.h>
#include <util/ring.h>
#include <util/json.h>
#include "ni.h"
#include "malloc.h"
#include "gmalloc.h"
#include "mp.h"
#include "loader.h"
#include "icc.h"
#include "event.h"
#include "cpu.h"
#include "vm.h"
#include "stdio.h"
#include "httpd.h"

#include "manager.h"

static uint32_t manager_ip	= 0xc0a8c8fe;	// 192.168.200.254
static uint32_t manager_gw	= 0xc0a8c8c8;	// 192.168.200.200
//static uint32_t manager_ip	= 0xc0a864fe;	// 192.168.100.254
//static uint32_t manager_gw	= 0xc0a864c8;	// 192.168.100.200
static uint32_t manager_netmask= 0xffffff00;	// 255.255.255.0

static NI* manager_ni;
static struct netif* manager_netif;

static uint64_t	last_pid = 1;
static Map*	vms;

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

static Core cores[MP_MAX_CORE_COUNT];

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

static void icc_started(ICC_Message* msg) {
	Core* core = &cores[msg->core_id];
	VM* vm = core->vm;
	
	if(msg->result == 0) {
		core->error_code = 0;
		core->status = VM_STATUS_STARTED;
		core->stdin = msg->data.execute.stdin;
		core->stdin_head = msg->data.execute.stdin_head;
		core->stdin_tail = msg->data.execute.stdin_tail;
		core->stdin_size = msg->data.execute.stdin_size;
		core->stdout = msg->data.execute.stdout;
		core->stdout_head = msg->data.execute.stdout_head;
		core->stdout_tail = msg->data.execute.stdout_tail;
		core->stdout_size = msg->data.execute.stdout_size;
		core->stderr = msg->data.execute.stderr;
		core->stderr_head = msg->data.execute.stderr_head;
		core->stderr_tail = msg->data.execute.stderr_tail;
		core->stderr_size = msg->data.execute.stderr_size;
		
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
	
	event_trigger(EVENT_VM_STARTED, vm);
	
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
	
	event_trigger(EVENT_VM_STOPPED, vm);
	
	printf("VM stopped on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

static int itoh(char* buf, int value) {
	#define HEX(v)	(((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
	
	if(value == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}
	
	int len = 0;
	while(value != 0) {
		buf[len++] = HEX(value);
		value >>= 4;
	}
	buf[len] = '\0';
	
	int len2 = len / 2;
	for(int i = 0; i < len2; i++) {
		char t = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = t;
	}
	
	return len;
}

static void idle_io(NetworkInterface* ni) {
	ni_poll();
	
	int get_thread_id(VM* vm, int core) {
		for(int i = 0; i < vm->core_size; i++) {
			if(vm->cores[i] == core)
				return i;
		}
		
		return -1;
	}
	
	void output(uint64_t pid, HTTPContext* res, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
		int text_len = ring_readable(*head, *tail, size);
		
		char size_buf[8];
		int size_len = itoh(size_buf, text_len);
		
		char thread_buf[8];
		int thread_len = itoh(thread_buf, thread_id);
		
		char fd_buf[8];
		int fd_len = itoh(fd_buf, fd);
		
		int content_len = thread_len + 2 + fd_len + 2 + size_len + 2 + text_len;
		
		char total_buf[8];
		int total_len = itoh(total_buf, content_len);
		
		int total = total_len + 2 + content_len + 2;
		
		int available = HTTP_BUFFER_SIZE - res->buf.index;
		if(available < (total_len + 2 + fd_len + 2 + size_len + 2) * 2)
			return;
		
		if(available < total) {
			text_len -= total - available;
			size_len = itoh(size_buf, text_len);
			content_len = thread_len + 2 + fd_len + 2 + size_len + 2 + text_len;
			total_len = itoh(total_buf, content_len);
			total = total_len + 2 + content_len + 2;
		}
		
		// TODO: Change it using sprintf
		char* buf = (char*)res->buf.buf + res->buf.index;
		res->buf.index += total;
		
		memcpy(buf, total_buf, total_len); buf += total_len;
		memcpy(buf, "\r\n", 2); buf += 2;
		
		memcpy(buf, thread_buf, thread_len); buf += thread_len;
		memcpy(buf, "\r\n", 2); buf += 2;
		
		memcpy(buf, fd_buf, fd_len); buf += fd_len;
		memcpy(buf, "\r\n", 2); buf += 2;
		
		memcpy(buf, size_buf, size_len); buf += size_len;
		memcpy(buf, "\r\n", 2); buf += 2;
		
		ring_read(buffer, head, *tail, size, buf, text_len);
		buf += text_len;
		memcpy(buf, "\r\n", 2); buf += 2;
		
		httpd_send(res);
	}
	
	// Check Std I/O/E
	static int core_index;
	
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		core_index = (core_index + 1) % MP_MAX_CORE_COUNT;
		
		Core* core = &cores[core_index];
		
		if(core->status != VM_STATUS_STOPPED) {
			int thread_id = -1;
			
			if(core->vm->res != NULL && core->stdout != NULL && *core->stdout_head != *core->stdout_tail) {
				thread_id = get_thread_id(core->vm, core_index);
				
				output(core->vm->id, core->vm->res, thread_id, 1, core->stdout, core->stdout_head, core->stdout_tail, core->stdout_size);
			}
			
			if(core->vm->res != NULL && core->stderr != NULL && *core->stderr_head != *core->stderr_tail) {
				if(thread_id == -1)
					thread_id = get_thread_id(core->vm, core_index);
				
				output(core->vm->id, core->vm->res, thread_id, 2, core->stderr, core->stderr_head, core->stderr_tail, core->stderr_size);
			}
			
			break;
		}
	}
}

// Start of pseudo user space API
#include <time.h>
clock_t clock() {
	return (clock_t)(cpu_tsc() / (cpu_frequency / CLOCKS_PER_SEC));
}
// End of pseudo user space API

static NI* json_to_ni(JSONObject* obj) {
	uint64_t parse_mac(char* str) {
		uint64_t mac = 0;
		for(int i = 0; i < 6; i++) {
			char ch = str[2];
			str[2] = '\0';
			mac <<= 8;
			mac = mac | strtol(str, NULL, 16);
			str[2] = ch;
			str += 3;
		}
		
		return mac;
	}
	
	JSONAttr* attr = json_get(obj, "mac");
	if(!attr) return NULL;
	uint64_t mac = parse_mac(attr->data);
	if(mac == 0) {
		do {
			mac = cpu_tsc() & 0x0fffffffffffL;
			mac |= 0x02L << 40;	// Locally administrered
		} while(ni_contains(mac));
	} else if(ni_contains(mac)) {
		printf("Manager: The mac address already in use: %012x.\n", mac);
		errno = 509;
		return NULL;
	}
	
	attr = json_get(obj, "input_buffer_size");
	if(!attr) return NULL;
	uint32_t input_buffer_size = strtol(attr->data, NULL, 0);
	
	attr = json_get(obj, "output_buffer_size");
	if(!attr) return NULL;
	uint32_t output_buffer_size = strtol(attr->data, NULL, 0);
	
	attr = json_get(obj, "input_bandwidth");
	if(!attr) return NULL;
	uint64_t input_bandwidth = strtoll(attr->data, NULL, 0);
	
	attr = json_get(obj, "output_bandwidth");
	if(!attr) return NULL;
	uint64_t output_bandwidth = strtoll(attr->data, NULL, 0);
	
	attr = json_get(obj, "pool_size");
	if(!attr) return NULL;
	uint64_t pool_size = strtoll(attr->data, NULL, 0);
	
	uint64_t attrs[] = { 
		NI_MAC, mac,
		NI_INPUT_BUFFER_SIZE, input_buffer_size,
		NI_OUTPUT_BUFFER_SIZE, output_buffer_size,
		NI_INPUT_BANDWIDTH, input_bandwidth,
		NI_OUTPUT_BANDWIDTH, output_bandwidth,
		NI_POOL_SIZE, pool_size,
		NI_INPUT_ACCEPT_ALL, 1,
		NI_OUTPUT_ACCEPT_ALL, 1,
		NI_INPUT_FUNC, 0,
		NI_NONE
	};
	
	return ni_create(attrs);
}

static uint64_t json_to_vm(JSONObject* obj) {
	errno = 0;
	
	VM* vm = gmalloc(sizeof(VM));
	bzero(vm, sizeof(VM));
	
	// Allocate args
	JSONAttr* attr = json_get(obj, "args");
	if(attr) {
		if(attr->type != JSON_ARRAY) { errno = 400; return 0; }
		JSONArray* args = (JSONArray*)attr->data;
		
		int len = sizeof(char*) * args->size;
		for(int i = 0; i < args->size; i++) {
			if(args->array[i].type != JSON_STRING) {
				printf("Manager: Illegal args syntax.\n");
				errno = 400;
				vm_destroy(vm, -1);
				return 0;
			}
			
			len += strlen(args->array[i].data) + 1;
		}
		
		vm->argc = args->size;
		vm->argv = gmalloc(len);
		char* sbuf = (char*)vm->argv + sizeof(char*) * args->size;
		for(int i = 0; i < args->size; i++) {
			vm->argv[i] = sbuf;
			char* str = args->array[i].data;
			int slen = strlen(str) + 1;
			memcpy(sbuf, str, slen);
			sbuf += slen;
		}
	}
	
	// Allocate core
	attr = json_get(obj, "core_num");
	if(!attr) { errno = 400; return 0; }
	vm->core_size = strtol(attr->data, NULL, 0);
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
		errno = 507; // Not enough resource to allocate
		vm_destroy(vm, -1);
		return 0;
	}
	
	// Allocate memory
	attr = json_get(obj, "memory_size");
	if(!attr) { errno = 400; return 0; }
	uint64_t memory_size = strtoll(attr->data, NULL, 0);
	memory_size = (memory_size + (VM_MEMORY_SIZE_ALIGN - 1)) & ~(VM_MEMORY_SIZE_ALIGN - 1);
	vm->memory.count = memory_size / VM_MEMORY_SIZE_ALIGN;
	vm->memory.blocks = gmalloc(vm->memory.count * sizeof(void*));
	for(int i = 0; i < vm->memory.count; i++) {
		vm->memory.blocks[i] = bmalloc();
		if(!vm->memory.blocks[i]) {
			printf("Manager: Not enough memory to allocate.\n");
			errno = 507;
			vm_destroy(vm, -1);
			return 0;
		}
	}
	
	// Allocate storage
	attr = json_get(obj, "storage_size");
	if(!attr) { errno = 400; return 0; }
	uint64_t storage_size = strtoll(attr->data, NULL, 0);
	storage_size = (storage_size + (VM_STORAGE_SIZE_ALIGN - 1)) & ~(VM_STORAGE_SIZE_ALIGN - 1);
	vm->storage.count = storage_size / VM_STORAGE_SIZE_ALIGN;
	vm->storage.blocks = gmalloc(vm->storage.count * sizeof(void*));
	for(int i = 0; i < vm->storage.count; i++) {
		vm->storage.blocks[i] = bmalloc();
		if(!vm->storage.blocks[i]) {
			printf("Manager: Not enough storage to allocate.\n");
			errno = 507;
			vm_destroy(vm, -1);
			return 0;
		}
	}
	
	// Allocate NICs
	attr = json_get(obj, "nics");
	if(attr) {
		if(attr->type != JSON_ARRAY) { errno = 400; return 0; }
		JSONArray* nics = (JSONArray*)attr->data;
		
		vm->nic_size = nics->size;
		vm->nics = gmalloc(sizeof(NI) * vm->nic_size);
		
		for(int i = 0; i < nics->size; i++) {
			if(nics->array[i].type != JSON_OBJECT) {
				errno = 400;
				vm_destroy(vm, -1);
				return -1;
			}
			
			vm->nics[i] = json_to_ni(nics->array[i].data);
			if(!vm->nics[i]) {
				if(errno == 0) {
					printf("Manager: Not enough NIC to allocate.\n");
					errno = 507;
				}
				
				vm_destroy(vm, -1);
				return 0;
			}
		}
	}
	
	// Allocate pid
	uint64_t pid;
	while(true) {
		pid = last_pid++;
		
		if(pid != 0 && !map_contains(vms, (void*)pid)) {
			vm->id = pid;
			map_put(vms, (void*)pid, vm);
			break;
		}
	}
	
	// Dump info
	printf("Manager: VM[%d] created(cores [", pid);
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
	
	return pid;
}

static bool vm_create(HTTPContext* req, HTTPContext* res) {
	if(req->buf.total_index < req->buf.total_length)
		return true;
	
	JSONType* json = json_parse((char*)req->buf.buf, req->buf.total_length);
	if(json) {
		uint64_t pid = json_to_vm((JSONObject*)json);
		if(pid == 0) {
			switch(errno) {
				case 400:
					httpd_status_set(res, errno, "JSON syntax error");
					break;
				case 507:
					httpd_status_set(res, errno, "Not enough resource to allocate");
					break;
				case 509:
					httpd_status_set(res, errno, "The resource already allocated");
					break;
				default:
					httpd_status_set(res, 500, "Internal error");
			}
		} else {
			res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "%d", pid);
			
			httpd_header_put(res, "Content-Type", "application/json");
			httpd_status_set(res, 200, "OK");
		}
		
		json_free(json);
	} else {
		res->buf.index = 0;
		res->buf.total_length = 0;
		
		httpd_status_set(res, 400, "JSON syntax error");
	}
	
	httpd_send(res);
	
	return true;
}

static bool vm_delete(HTTPContext* req, HTTPContext* res) {
	if(req->buf.total_index < req->buf.total_length)
		return true;
	
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status == VM_STATUS_STARTED) {
			httpd_status_set(res, 401, "Permission denied");
			httpd_send(res);
			return true;
		}
	}
	
	printf("Manager: Delete vm[%d] on cores [", pid);
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", vm->cores[i]);
		if(i + 1 < vm->core_size)
			printf(", ");
	}
	printf("]\n");
	
	map_remove(vms, (void*)pid);
	vm_destroy(vm, -1);
	
	res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "true");
	
	httpd_header_put(res, "Content-Type", "application/json");
	httpd_status_set(res, 200, "OK");
	httpd_send(res);
	
	return true;
}

static bool vm_power_set(HTTPContext* req, HTTPContext* res) {
	typedef struct {
		HTTPContext*	res;
		bool		is_closed;
		uint64_t	listener_id;
	} Data;
	
	bool start_callback(int event_id, void* context, void* event) {
		Data* data = context;
		
		if(data->is_closed) {
			free(data);
			return false;
		}
		
		VM* vm = event;
		HTTPContext* res = data->res;
		
		httpd_close_listener_remove(res, data->listener_id);
		
		free(data);
		
		if(vm->status == VM_STATUS_STARTED) {
			res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "true");
			
			httpd_header_put(res, "Content-Type", "application/json");
			httpd_status_set(res, 200, "OK");
			httpd_send(res);
			
			return false;
		} else {
			int error_code = 0;
			for(int i = 0; i < vm->core_size; i++) {
				Core* core = &cores[vm->cores[i]];
				if(core->status == VM_STATUS_STARTED)
					continue;
				
				error_code = core->error_code;
				break;
			}
			
			res->buf.total_length = res->buf.index = sprintf((char*)res->buf.buf, "%d", error_code);
			
			httpd_header_put(res, "Content-Type", "application/json");
			httpd_status_set(res, 500, "Internal error");
			httpd_send(res);
			
			return false;
		}
	}
	
	bool start_close_listener(HTTPContext* req, HTTPContext* res, void* context) {
		Data* data = context;
		data->is_closed = true;
		return false;
	}
	
	void start(uint64_t pid, VM* vm) {
		printf("Manager: VM[%d] starts on cores [", pid);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("]\n");
		
		for(int i = 0; i < vm->core_size; i++) {
			cores[vm->cores[i]].error_code = 0;
			ICC_Message* msg = icc_sending(ICC_TYPE_START, vm->cores[i]);
			msg->data.start.vm = vm;
			icc_send(msg);
		}
		
		Data* data = malloc(sizeof(Data));
		bzero(data, sizeof(Data));
		data->res = res;
		data->listener_id = httpd_close_listener_add(res, start_close_listener, data);
		event_event(EVENT_VM_STARTED, start_callback, data);
	}
	
	bool stop_callback(int event_id, void* context, void* event) {
		Data* data = context;
		
		if(data->is_closed) {
			free(data);
			return false;
		}
		
		HTTPContext* res = data->res;
		
		httpd_close_listener_remove(res, data->listener_id);
		
		free(data);
		
		res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "true");
		
		httpd_header_put(res, "Content-Type", "application/json");
		httpd_status_set(res, 200, "OK");
		httpd_send(res);
		
		return false;
	}
	
	bool stop_close_listener(HTTPContext* req, HTTPContext* res, void* context) {
		Data* data = context;
		data->is_closed = true;
		return false;
	}
	
	void stop(uint64_t pid, VM* vm) {
		printf("Manager: VM[%d] stops on cores [", pid);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", vm->cores[i]);
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("]\n");
		
		Data* data = malloc(sizeof(Data));
		bzero(data, sizeof(Data));
		data->res = res;
		
		for(int i = 0; i < vm->core_size; i++) {
			if(cores[vm->cores[i]].status == VM_STATUS_STARTED) {
				ICC_Message* msg = icc_sending(ICC_TYPE_STOP, vm->cores[i]);
				icc_send(msg);
			}
		}
		
		data->listener_id = httpd_close_listener_add(res, stop_close_listener, data);
		event_event(EVENT_VM_STOPPED, stop_callback, data);
	}
	
	if(req->buf.total_index < req->buf.total_length)
		return true;
	
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	bool is_changed = false;
	
	if(req->buf.total_length == 4 && strncmp((char*)req->buf.buf, "stop", 4) == 0) {
		switch(vm->status) {
			case VM_STATUS_STOPPED:
				// Do nothing
				break;
			case VM_STATUS_PAUSED:
				// TODO: Do something
				break;
			case VM_STATUS_STARTED:
				stop(pid, vm);
				return true;
		}
	} else if(req->buf.total_length == 5 && strncmp((char*)req->buf.buf, "pause", 5) == 0) {
		switch(vm->status) {
			case VM_STATUS_STOPPED:
				// TODO: Do something
				break;
			case VM_STATUS_PAUSED:
				// Do nothing
				break;
			case VM_STATUS_STARTED:
				// TODO: Do something
				break;
		}
	} else if(req->buf.total_length == 5 && strncmp((char*)req->buf.buf, "start", 5) == 0) {
		switch(vm->status) {
			case VM_STATUS_STOPPED:
				start(pid, vm);
				return true;
			case VM_STATUS_PAUSED:
				start(pid, vm);
				return true;
			case VM_STATUS_STARTED:
				// Do nothing
				break;
		}
	} else {
		httpd_status_set(res, 400, "JSON syntax error");
		httpd_send(res);
		return true;
	}
	
	if(is_changed) {
		res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "true");
		
		httpd_header_put(res, "Content-Type", "application/json");
		httpd_status_set(res, 200, "OK");
		httpd_send(res);
	} else {
		res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "false");
		
		httpd_header_put(res, "Content-Type", "application/json");
		httpd_status_set(res, 200, "OK");
		httpd_send(res);
	}
	
	return true;
}

static bool vm_power_get(HTTPContext* req, HTTPContext* res) {
	if(req->buf.total_index < req->buf.total_length)
		return true;
	
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	char* status;
	switch(vm->status) {
		case VM_STATUS_STOPPED:
			status = "stopped";
			break;
		case VM_STATUS_PAUSED:
			status = "paused";
			break;
		case VM_STATUS_STARTED:
			status = "started";
			break;
		default:
			status = "zombie";
	}
	
	res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, status);
	
	httpd_header_put(res, "Content-Type", "application/json");
	httpd_status_set(res, 200, "OK");
	httpd_send(res);
	
	return true;
}

static bool vm_storage_set(HTTPContext* req, HTTPContext* res) {
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	if(vm->status == VM_STATUS_STARTED) {
		httpd_status_set(res, 401, "Permission denied");
		httpd_send(res);
		return true;
	}
	
	int index0 = req->buf.total_index - req->buf.index;
	int size = req->buf.index;
	
	int block = index0 / VM_STORAGE_SIZE_ALIGN;
	int index = index0 % VM_STORAGE_SIZE_ALIGN;
	
	if(index + size > VM_STORAGE_SIZE_ALIGN) {
		if(block + 1 >= vm->storage.count) {
			httpd_status_set(res, 507, "Not enough resource to allocate");
			httpd_send(res);
			return true;
		}
		
		int len2 = index + size - VM_STORAGE_SIZE_ALIGN;
		int len1 = VM_STORAGE_SIZE_ALIGN - len2;
		memcpy(vm->storage.blocks[block] + index, req->buf.buf, len1);
		memcpy(vm->storage.blocks[block + 1], req->buf.buf + len1, len2);
	} else {
		if(block >= vm->storage.count) {
			httpd_status_set(res, 507, "Not enough resource to allocate");
			httpd_send(res);
			return true;
		}
		
		memcpy(vm->storage.blocks[block] + index, req->buf.buf, size);
	}
	
	req->buf.index = 0;
	
	if(req->buf.total_index == 0) {
		printf("Manager: Uploading vm[%d] storage: %ld bytes\n", pid, req->buf.total_length);
	} else {
		printf(".");
	}
	
	if(req->buf.total_index >= req->buf.total_length) {
		printf("\n");
		
		uint32_t md5sum[4];
		md5_blocks(vm->storage.blocks, vm->storage.count, VM_STORAGE_SIZE_ALIGN, req->buf.total_length, md5sum);
		uint8_t* p = (uint8_t*)md5sum;
		
		#define HEX(v)  (((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
		uint8_t* c = res->buf.buf;
		for(int i = 0; i < 16; i++) {
			*c++ = HEX(p[i] >> 4);
			*c++ = HEX(p[i] >> 0);
		}
		
		res->buf.index = 32;
		res->buf.total_length = 32;
		
		httpd_header_put(res, "Content-Type", "plain/text");
		httpd_status_set(res, 200, "OK");
		httpd_send(res);
	}
	
	return true;
}

static bool vm_storage_get(HTTPContext* req, HTTPContext* res) {
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	bool send(HTTPContext* req, HTTPContext* res, void* context) {
		VM* vm = context;
		
		if(res->buf.index + 4096 > HTTP_BUFFER_SIZE) {
			httpd_send(res);
			return true;
		}
		
		uint64_t total = res->buf.total_index + res->buf.index;
		int block = total / VM_STORAGE_SIZE_ALIGN;
		int index = total % VM_STORAGE_SIZE_ALIGN;
		
		memcpy(res->buf.buf + res->buf.index, vm->storage.blocks[block] + index, 4096);
		res->buf.index += 4096;
		httpd_send(res);
		
		printf(".");
		
		if(res->buf.total_index >= res->buf.total_length) {
			printf("\n");
			return false;
		} else {
			return true;
		}
	}
	
	res->buf.total_length = vm->storage.count * VM_STORAGE_SIZE_ALIGN;
	httpd_header_put(res, "Content-Type", "application/octet-stream");
	httpd_status_set(res, 200, "OK");
	printf("Manager: Downloading vm[%d] storage: %ld bytes\n", pid, res->buf.total_length);
	
	httpd_send_listener_add(res, send, vm);
	send(req, res, vm);
	
	return true;
}

static bool vm_storage_delete(HTTPContext* req, HTTPContext* res) {
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	printf("Manager: Deleting vm[%d] storage: %ld bytes\n", pid, vm->storage.count * VM_STORAGE_SIZE_ALIGN);
	httpd_header_put(res, "Content-Type", "application/json");
	httpd_status_set(res, 200, "OK");
	
	for(int i = 0; i < vm->storage.count; i++) {
		bzero(vm->storage.blocks[i], VM_STORAGE_SIZE_ALIGN);
	}
	
	res->buf.index = res->buf.total_length = sprintf((char*)res->buf.buf, "true");
	
	httpd_send(res);
	
	return true;
}

static bool vm_stdio(HTTPContext* req, HTTPContext* res) {
	bool close_listener(HTTPContext* req, HTTPContext* res, void* context) {
		((VM*)context)->res = NULL;
		
		return false;
	}
	
	uint8_t* find_eol(uint8_t* buf, uint8_t* end) {
		for(; buf <= end - 2; buf++) {
			if(buf[0] == '\r' && buf[1] == '\n')
				return buf + 2;
		}
		return NULL;
	}
	
	uint64_t pid = strtoll(httpd_param_get(req, "pid"), NULL, 0);
	VM* vm = map_get(vms, (void*)pid);
	if(!vm) {
		httpd_status_set(res, 410, "There is no such resource");
		httpd_send(res);
		return true;
	}
	
	if(vm->res != res) {
		if(vm->res) {
			httpd_close(vm->res);
		}
		
		httpd_close_listener_add(res, close_listener, vm);
		res->encoding = HTTP_ENCODING_CHUNKED;
		httpd_status_set(res, 200, "OK");
		httpd_send(res);
		
		vm->res = res;
	}
	
	int len = req->buf.index - req->buf.total_index + req->buf.total_length;
	uint8_t* buf = req->buf.buf;
	uint8_t* last = buf;
	uint8_t* end = buf + len;
	
	while(buf < end) {
		int thread_id = strtol((const char*)buf, NULL, 16);
		
		buf = find_eol(buf, end);
		if(!buf)
			goto done;
		int fd = strtol((const char*)buf, NULL, 16);
		
		buf = find_eol(buf, end);
		if(!buf)
			goto done;
		int size = strtol((const char*)buf, NULL, 16);
		
		buf = find_eol(buf, end);
		if(!buf)
			goto done;
		
		if(end - buf < size)
			goto done;
		
		if(fd == 0 && thread_id < vm->core_size) {
			Core* core = &cores[vm->cores[thread_id]];
			
			ssize_t len = ring_write(core->stdin, *core->stdin_head, core->stdin_tail, core->stdin_size, (char*)buf, size);
			if(len < size) {
				int text_len = size - len;
				
				char size_buf[8];
				int size_len = itoh(size_buf, text_len);
				
				char thread_buf[8];
				int thread_len = itoh(thread_buf, thread_id);
				
				char fd_buf[8];
				int fd_len = itoh(fd_buf, fd);
				
				int content_len = fd_len + 2 + thread_len + 2 + size_len + 2 + text_len;
				
				char total_buf[8];
				int total_len = itoh(total_buf, content_len);
				
				int total = total_len + 2 + content_len;
				
				buf -= total;
				last = buf;
				
				memcpy(buf, total_buf, total_len); buf += total_len;
				memcpy(buf, "\r\n", 2); buf += 2;
				
				memcpy(buf, thread_buf, thread_len); buf += thread_len;
				memcpy(buf, "\r\n", 2); buf += 2;
				
				memcpy(buf, fd_buf, fd_len); buf += fd_len;
				memcpy(buf, "\r\n", 2); buf += 2;
				
				memcpy(buf, size_buf, size_len); buf += size_len;
				memcpy(buf, "\r\n", 2); buf += 2;
				
				goto done;
			}
		}
		
		buf += size;
		last = buf;
	}
	
done:
	memmove(req->buf.buf, last, end - last);
	int total = last - req->buf.buf;
	req->buf.index -= total;
	
	return true;
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
	
	extern NetworkInterface* __nis;
	__nis = manager_ni->ni;
	extern int __nis_count;
	__nis_count = 1;
	
	manager_netif = ni_init(0, manager_ip, manager_netmask, manager_gw, true, NULL, NULL);
	
	HTTPD* httpd = httpd_create(0, 80);
	httpd_post(httpd, "/vm", vm_create);
	httpd_delete(httpd, "/vm/:pid", vm_delete);
	httpd_put(httpd, "/vm/:pid/power", vm_power_set);
	httpd_get(httpd, "/vm/:pid/power", vm_power_get);
	httpd_post(httpd, "/vm/:pid/storage", vm_storage_set);
	httpd_get(httpd, "/vm/:pid/storage", vm_storage_get);
	httpd_delete(httpd, "/vm/:pid/storage", vm_storage_delete);
	httpd_post(httpd, "/vm/:pid/stdio", vm_stdio);
	
	event_idle((void*)idle_io, manager_ni->ni);
	
	vms = map_create(3, map_uint64_hash, map_uint64_equals, malloc, free);
	
	icc_register(ICC_TYPE_STARTED, icc_started);
	icc_register(ICC_TYPE_STOPPED, icc_stopped);
	
	// Core 0 is occupied by manager
	cores[0].status = VM_STATUS_STARTED;
}

void manager_get_ip(uint32_t* ip, uint32_t* netmask, uint32_t* gateway) {
	// TODO: Implement it
}

void manager_set_ip(uint32_t ip, uint32_t netmask, uint32_t gateway) {
	// TODO: Implement it
}
