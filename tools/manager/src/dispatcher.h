#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <linux/types.h>

#define DISPATCHER			0xAC

#define DISPATCHER_CREATE_NIC		_IOW(DISPATCHER, 0x10, void *)
#define DISPATCHER_DESTROY_NIC		_IOW(DISPATCHER, 0x11, void *)

#define DISPATCHER_CREATE_VNIC		_IOW(DISPATCHER, 0x20, void *)
#define DISPATCHER_DESTROY_VNIC		_IOW(DISPATCHER, 0x21, void *)
#define DISPATCHER_UPDATE_VNIC		_IOWR(DISPATCHER, 0x22, void *)
#define DISPATCHER_GET_VNIC		_IOR(DISPATCHER, 0x23, void *)

int dispatcher_init(void);
int dispatcher_exit(void);

int dispatcher_create_nic(void* nic);
int dispatcher_destroy_nic(void* nic);

int dispatcher_create_vnic(void* vnic);
int dispatcher_destroy_vnic(void* vnic);
int dispatcher_update_vnic(void* vnic);
int dispatcher_get_vnic(void* vnic);

#endif /* __DISPATCHER_H__ */
