#ifndef __CONTROL_RPC__
#define __CONTROL_RPC__

#include <util/list.h>
#include <control/vmspec.h>

#define RPC_MAGIC		"PNRPC"
#define RPC_MAGIC_SIZE		5
#define RPC_VERSION		1
#define RPC_BUFFER_SIZE		8192

typedef enum {
	RPC_TYPE_HELLO_REQ = 1,
	RPC_TYPE_HELLO_RES,
	RPC_TYPE_VM_CREATE_REQ,
	RPC_TYPE_VM_CREATE_RES,
	RPC_TYPE_VM_GET_REQ,
	RPC_TYPE_VM_GET_RES,
	RPC_TYPE_VM_SET_REQ,
	RPC_TYPE_VM_SET_RES,
	RPC_TYPE_VM_DELETE_REQ,
	RPC_TYPE_VM_DELETE_RES,		// 10
	RPC_TYPE_VM_LIST_REQ,
	RPC_TYPE_VM_LIST_RES,
	RPC_TYPE_STATUS_GET_REQ,
	RPC_TYPE_STATUS_GET_RES,
	RPC_TYPE_STATUS_SET_REQ,
	RPC_TYPE_STATUS_SET_RES,
	RPC_TYPE_STORAGE_DOWNLOAD_REQ,
	RPC_TYPE_STORAGE_DOWNLOAD_RES,
	RPC_TYPE_STORAGE_UPLOAD_REQ,
	RPC_TYPE_STORAGE_UPLOAD_RES,	// 20
	RPC_TYPE_STORAGE_MD5_REQ,
	RPC_TYPE_STORAGE_MD5_RES,
	RPC_TYPE_STDIO_REQ,
	RPC_TYPE_STDIO_RES,
	RPC_TYPE_END,			// 22
} RPC_TYPE;

typedef struct _RPC RPC;

struct _RPC {
	// Connection information
	int		ver;
	
	// I/O
	int(*read)(RPC* rpc, void* buf, int size);
	int(*write)(RPC* rpc, void* buf, int size);
	void(*close)(RPC* rpc);
	
	uint8_t		rbuf[RPC_BUFFER_SIZE];
	int		rbuf_read;
	int		rbuf_index;
	
	uint8_t		wbuf[RPC_BUFFER_SIZE];
	int		wbuf_index;
	
	// Callbacks
	bool(*hello_callback)(void* context);
	void* hello_context;
	bool(*vm_create_callback)(uint32_t id, void* context);
	void* vm_create_context;
	void(*vm_create_handler)(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, uint32_t id));
	void* vm_create_handler_context;
	bool(*vm_get_callback)(VMSpec* vm, void* context);
	void* vm_get_context;
	void(*vm_get_handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, VMSpec* vm));
	void* vm_get_handler_context;
	bool(*vm_set_callback)(bool result, void* context);
	void* vm_set_context;
	void(*vm_set_handler)(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, bool result));
	void* vm_set_handler_context;
	bool(*vm_destroy_callback)(bool result, void* context);
	void* vm_destroy_context;
	void(*vm_destroy_handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, bool result));
	void* vm_destroy_handler_context;
	bool(*vm_list_callback)(uint32_t* ids, uint16_t count, void* context);
	void* vm_list_context;
	void(*vm_list_handler)(RPC* rpc, int size, void* context, void(*callback)(RPC* rpc, uint32_t* ids, int size));
	void* vm_list_handler_context;
	bool(*status_get_callback)(VMStatus status, void* context);
	void* status_get_context;
	void(*status_get_handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, VMStatus status));
	void* status_get_handler_context;
	bool(*status_set_callback)(bool result, void* context);
	void* status_set_context;
	void(*status_set_handler)(RPC* rpc, uint32_t id, VMStatus status, void* context, void(*callback)(RPC* rpc, bool result));
	void* status_set_handler_context;
	int32_t(*storage_download_callback)(uint32_t offset, void* buf, int32_t size, void* context);
	void* storage_download_context;
	uint32_t storage_download_id;
	uint64_t storage_download_size;
	uint32_t storage_download_offset;
	void(*storage_download_handler)(RPC* rpc, uint32_t id, uint64_t download_size, uint32_t offset, int32_t size, void* context, void(*callback)(RPC* rpc, void* buf, int32_t size));
	void* storage_download_handler_context;
	uint32_t storage_upload_id;
	uint32_t storage_upload_offset;
	int32_t(*storage_upload_callback)(uint32_t offset, void** buf, int32_t size, void* context);
	void* storage_upload_context;
	void(*storage_upload_handler)(RPC* rpc, uint32_t id, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(RPC* rpc, int32_t size));
	void* storage_upload_handler_context;
	bool(*storage_md5_callback)(bool result, uint32_t md5[], void* context);
	void* storage_md5_context;
	void(*storage_md5_handler)(RPC* rpc, uint32_t id, uint64_t size, void* context, void(*callback)(RPC* rpc, bool result, uint32_t md5[]));
	void* storage_md5_handler_context;
	bool(*stdio_callback)(uint16_t written, void* context);
	void* stdio_context;
	void(*stdio_handler)(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size));
	void* stdio_handler_context;
	
	// Private data
	uint8_t		data[0];
};

// Client side APIs
#ifdef LINUX
RPC* rpc_open(const char* host, int port, int timeout);
RPC* rpc_listen(int port);
RPC* rpc_accept(RPC* rpc);
bool rpc_is_closed(RPC* rpc);
#endif /* LINUX */

int rpc_hello(RPC* rpc, bool(*callback)(void* context), void* context);

int rpc_vm_create(RPC* rpc, VMSpec* vm, bool(*callback)(uint32_t id, void* context), void* context);
int rpc_vm_get(RPC* rpc, uint32_t id, bool(*callback)(VMSpec* vm, void* context), void* context);
int rpc_vm_set(RPC* rpc, VMSpec* vm, bool(*callback)(bool result, void* context), void* context);
int rpc_vm_destroy(RPC* rpc, uint32_t id, bool(*callback)(bool result, void* context), void* context);
int rpc_vm_list(RPC* rpc, bool(*callback)(uint32_t* ids, uint16_t count, void* context), void* context);

int rpc_status_get(RPC* rpc, uint32_t id, bool(*callback)(VMStatus status, void* context), void* context);
int rpc_status_set(RPC* rpc, uint32_t id, VMStatus status, bool(*callback)(bool result, void* context), void* context);

int rpc_storage_download(RPC* rpc, uint32_t id, uint64_t size, int32_t(*callback)(uint32_t offset, void* buf, int32_t size, void* context), void* context);
int rpc_storage_upload(RPC* rpc, uint32_t id, int32_t(*callback)(uint32_t offset, void** buf, int32_t size, void* context), void* context);
void rpc_storage_md5(RPC* rpc, uint32_t id, uint64_t size, bool(*callback)(bool result, uint32_t md5[], void* context), void* context);

int rpc_stdio(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, const char* str, uint16_t size, bool(*callback)(uint16_t written, void* context), void* context);

// Server side APIs
void rpc_vm_create_handler(RPC* rpc, void(*handler)(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, uint32_t id)), void* context);
void rpc_vm_get_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, VMSpec* vm)), void* context);
void rpc_vm_set_handler(RPC* rpc, void(*handler)(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, bool result)), void* context);
void rpc_vm_destroy_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, bool result)), void* context);
void rpc_vm_list_handler(RPC* rpc, void(*handler)(RPC* rpc, int size, void* context, void(*callback)(RPC* rpc, uint32_t* ids, int size)), void* context);

void rpc_status_get_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, VMStatus status)), void* context);
void rpc_status_set_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, VMStatus status, void* context, void(*callback)(RPC* rpc, bool result)), void* context);

void rpc_storage_download_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint64_t download_size, uint32_t offset, int32_t size, void* context, void(*callback)(RPC* rpc, void* buf, int32_t size)), void* context);
void rpc_storage_upload_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(RPC* rpc, int32_t size)), void* context);
void rpc_storage_md5_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint64_t size, void* context, void(*callback)(RPC* rpc, bool result, uint32_t md5[])), void* context);

void rpc_stdio_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size)), void* context);

bool rpc_is_active(RPC* rpc);
bool rpc_loop(RPC* rpc);

// Utility
void rpc_vm_dump(VMSpec* vm);
 
#endif /* __CONTROL_RPC__ */
