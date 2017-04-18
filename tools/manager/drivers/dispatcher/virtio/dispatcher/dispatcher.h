#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <linux/types.h> 

#define DISPATCHER			0xAC

#define DISPATCHER_SET_MANAGER		_IOW(DISPATCHER, 0x00, pid_t)
#define DISPATCHER_UNSET_MANAGER	_IO(DISPATCHER, 0x01)

#define DISPATCHER_REGISTER_NIC		_IO(DISPATCHER, 0x10)
#define DISPATCHER_UNREGISTER_NIC	_IOW(DISPATCHER, 0x11, void *)

typedef void (*dispatcher_work_fn_t)(void *data);

struct dispatcher_work {
	struct list_head	node;
	dispatcher_work_fn_t	fn;
	void*			data;
	struct net_device*	dev;
};

int dispatcher_init(struct net_device *dev);
void dispatcher_exit(void);

extern bool dispatcher_enabled;

#endif /* __DISPATCHER_H__ */
