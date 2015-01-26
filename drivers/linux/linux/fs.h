#ifndef __LINUX_FS_H__
#define __LINUX_FS_H__

#include <linux/module.h>

struct file {
	void*		private_data;
};

struct inode {
	union {
		struct cdev		*i_cdev;
	};
};

struct file_operations {
	struct module *owner;
	
	int (*open) (struct inode *, struct file *);
	int (*release) (struct inode *, struct file *);
	
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
};

int alloc_chrdev_region(dev_t *, unsigned, unsigned, const char *);
void unregister_chrdev_region(dev_t, unsigned);

#endif /* __LINUX_FS_H__ */

