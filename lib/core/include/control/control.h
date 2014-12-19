#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "vm.h"

/**
 * errno -1: Timeout.
 * errno -2: Disconnected.
 * errno -3: Poll error.
 * errno -4: Cannot resolve the host name.
 * errno -5: Cannot make a connection to the host.
 * errno -6: Cannot connect to the host.
 * errno -7: Cannot register callback service.
 * errno -8: Cannot make receiver url.
 * errno -9: Cannot open file.
 * errno -10: Cannot get file infomation.
 * errno -11: MD5 Length is zero.
 * errno -12: Cannot make udp service.
 **/

bool ctrl_connect(char* host, int port, void(*connected)(bool result), void(*disconnected)());
bool ctrl_disconnect();
bool ctrl_ping(int count, void(*callback)(int count, clock_t delay));

bool ctrl_vm_create(VMSpec* vm, void(*callback)(uint64_t vmid));
bool ctrl_vm_get(uint64_t vmid, void(*callback)(uint64_t vmid, VMSpec* vm));
bool ctrl_vm_set(uint64_t vmid, VMSpec* vm, void(*callback)(bool result));
bool ctrl_vm_delete(uint64_t vmid, void(*callback)(uint64_t vmid, bool result));
bool ctrl_vm_list(void(*callback)(uint64_t* vmids, int count));

bool ctrl_storage_upload(uint64_t vmid, char* path, void(*progress)(uint64_t vmid, uint32_t progress, uint32_t total));
bool ctrl_storage_download(uint64_t vmid, uint32_t size, char* path, void(*progress)(uint64_t vmid, uint32_t progress, uint32_t total));
bool ctrl_storage_md5(uint64_t vmid, uint64_t size, void(*callback)(uint64_t vmid, uint32_t md5[4]));

bool ctrl_status_set(uint64_t vmid, int status, void(*callback)(uint64_t vmid, int status));
bool ctrl_status_get(uint64_t vmid, void(*callback)(uint64_t vmid, int status));

bool ctrl_stdout(void(*out)(uint64_t vmid, uint32_t thread_id, char* msg, int count));
bool ctrl_stderr(void(*err)(uint64_t vmid, uint32_t thread_id, char* msg, int count));
bool ctrl_stdin(uint64_t vmid, uint32_t thread_id, char* msg, int count);

bool ctrl_poll();

#endif /* __CONTROL_H__ */
