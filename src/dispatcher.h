#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <sys/types.h> 

#define DISPATCHER			0xAC

#define DISPATCHER_SET_MANAGER		_IOW(DISPATCHER, 0x00, pid_t)
#define DISPATCHER_UNSET_MANAGER	_IO(DISPATCHER, 0x01)

#define DISPATCHER_REGISTER_NIC		_IO(DISPATCHER, 0x10)
#define DISPATCHER_UNREGISTER_NIC	_IOW(DISPATCHER, 0x11, void *)

int dispatcher_init(void);
int dispatcher_exit(void);
int dispatcher_register_nic(void* nic);
int dispatcher_unregister_nic(void);


#endif /* __DISPATCHER_H__ */
