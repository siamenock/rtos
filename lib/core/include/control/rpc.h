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
	RPC_TYPE_VM_DELETE_RES,	// 10
	RPC_TYPE_VM_LIST_REQ,
	RPC_TYPE_VM_LIST_RES,
	RPC_TYPE_STATUS_GET_REQ,
	RPC_TYPE_STATUS_GET_RES,
	RPC_TYPE_STATUS_SET_REQ,
	RPC_TYPE_STATUS_SET_RES,
	RPC_TYPE_STORAGE_DOWNLOAD_REQ,
	RPC_TYPE_STORAGE_DOWNLOAD_RES,
	RPC_TYPE_STORAGE_UPLOAD_REQ,
	RPC_TYPE_END,	// 20
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
	void(*hello_callback)(void* context);
	void* hello_context;
	void(*vm_create_callback)(uint32_t id, void* context);
	void* vm_create_context;
	uint32_t(*vm_create_handler)(VMSpec* vm, void* context);
	void* vm_create_handler_context;
	void(*vm_get_callback)(VMSpec* vm, void* context);
	void* vm_get_context;
	VMSpec*(*vm_get_handler)(uint32_t id, void* context);
	void* vm_get_handler_context;
	void(*vm_set_callback)(bool result, void* context);
	void* vm_set_context;
	bool(*vm_set_handler)(VMSpec* vm, void* context);
	void* vm_set_handler_context;
	void(*vm_delete_callback)(bool result, void* context);
	void* vm_delete_context;
	bool(*vm_delete_handler)(uint32_t id, void* context);
	void* vm_delete_handler_context;
	void(*vm_list_callback)(uint32_t* ids, uint16_t count, void* context);
	void* vm_list_context;
	int(*vm_list_handler)(uint32_t* ids, int size, void* context);
	void* vm_list_handler_context;
	void(*status_get_callback)(VMStatus status, void* context);
	void* status_get_context;
	VMStatus(*status_get_handler)(uint32_t id, void* context);
	void* status_get_handler_context;
	void(*status_set_callback)(bool result, void* context);
	void* status_set_context;
	bool(*status_set_handler)(uint32_t id, VMStatus status, void* context);
	void* status_set_handler_context;
	uint16_t(*storage_download_callback)(uint32_t offset, void* buf, uint16_t size, void* context);
	void* storage_download_context;
	uint32_t storage_download_id;
	uint32_t storage_download_offset;
	uint16_t(*storage_download_handler)(uint32_t id, uint32_t offset, void** buf, uint16_t size, void* context);
	void* storage_download_handler_context;
	uint32_t storage_upload_id;
	uint32_t storage_upload_offset;
	uint16_t(*storage_upload_callback)(uint32_t offset, void** buf, uint16_t size, void* context);
	void* storage_upload_context;
	uint16_t(*storage_upload_handler)(uint32_t id, uint32_t offset, void* buf, uint16_t size, void* context);
	void* storage_upload_handler_context;
	
	// Private data
	uint8_t		data[0];
};

// Client side APIs
#ifdef LINUX
RPC* rpc_open(const char* host, int port);
RPC* rpc_listen(int port);
RPC* rpc_accept(RPC* rpc);
bool rpc_is_closed(RPC* rpc);
#endif /* LINUX */

int rpc_hello(RPC* rpc, void(*callback)(void* context), void* context);

int rpc_vm_create(RPC* rpc, VMSpec* vm, void(*callback)(uint32_t id, void* context), void* context);
int rpc_vm_get(RPC* rpc, uint32_t id, void(*callback)(VMSpec* vm, void* context), void* context);
int rpc_vm_set(RPC* rpc, VMSpec* vm, void(*callback)(bool result, void* context), void* context);
int rpc_vm_delete(RPC* rpc, uint32_t id, void(*callback)(bool result, void* context), void* context);
int rpc_vm_list(RPC* rpc, void(*callback)(uint32_t* ids, uint16_t count, void* context), void* context);

int rpc_status_get(RPC* rpc, uint32_t id, void(*callback)(VMStatus status, void* context), void* context);
int rpc_status_set(RPC* rpc, uint32_t id, VMStatus status, void(*callback)(bool result, void* context), void* context);

int rpc_storage_download(RPC* rpc, uint32_t id, uint16_t(*callback)(uint32_t offset, void* buf, uint16_t size, void* context), void* context);
int rpc_storage_upload(RPC* rpc, uint32_t id, uint16_t(*callback)(uint32_t offset, void** buf, uint16_t size, void* context), void* context);

// Server side APIs
void rpc_vm_create_handler(RPC* rpc, uint32_t(*handler)(VMSpec* vm, void* context), void* context);
void rpc_vm_get_handler(RPC* rpc, VMSpec*(*handler)(uint32_t id, void* context), void* context);
void rpc_vm_set_handler(RPC* rpc, bool(*handler)(VMSpec* vm, void* context), void* context);
void rpc_vm_delete_handler(RPC* rpc, bool(*handler)(uint32_t id, void* context), void* context);
void rpc_vm_list_handler(RPC* rpc, int(*handler)(uint32_t* ids, int size, void* context), void* context);

void rpc_status_get_handler(RPC* rpc, VMStatus(*handler)(uint32_t id, void* context), void* context);
void rpc_status_set_handler(RPC* rpc, bool(*handler)(uint32_t id, VMStatus status, void* context), void* context);

void rpc_storage_download_handler(RPC* rpc, uint16_t(*handler)(uint32_t id, uint32_t offset, void** buf, uint16_t size, void* context), void* context);
void rpc_storage_upload_handler(RPC* rpc, uint16_t(*handler)(uint32_t id, uint32_t offset, void* buf, uint16_t size, void* context), void* context);

bool rpc_is_active(RPC* rpc);
bool rpc_loop(RPC* rpc);

// Utility
void rpc_vm_dump(VMSpec* vm);
 
#endif /* __CONTROL_RPC__ */
