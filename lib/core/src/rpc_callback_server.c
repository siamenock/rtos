#include <unistd.h>
#include "rpc_callback.h"

void* callback_null_1_svc(struct svc_req *rqstp) {
	static char* result;
	return (void*) &result;
}

void* stdio_1_svc(RPC_Message message,  struct svc_req *rqstp) {
	static char* result;
	
	static uint64_t last_vmid = (uint64_t)-1;
	static uint32_t last_coreid = (uint32_t)-1;
	static uint32_t last_fd = (uint32_t)-1;
	
	if(last_vmid != message.vmid || last_coreid != message.coreid || last_fd != message.fd) {
		char buf[80];
		int len = sprintf(buf, "***** vmid=%lu thread=%d standard %s *****\n", message.vmid, message.coreid, message.fd == 1 ? "output" : "error");
		write(0, buf, len);
		
		last_vmid = message.vmid;
		last_coreid = message.coreid;
		last_fd = message.fd;
	}
	
	write(0, message.message.message_val, message.message.message_len);
	
	return (void*)&result;
}
