#ifndef __LINUX_MODULE_H__
#define __LINUX_MODULE_H__

#include <linux/moduleparam.h>

struct module __this_module;

#define THIS_MODULE 				(&__this_module)
#define MODULE_INFO(tag, info)
#define MODULE_NAME_LEN				(64 - sizeof(unsigned long))
#define MODULE_LICENSE(_license)		MODULE_INFO(license, _license)
#define MODULE_AUTHOR(_author)			MODULE_INFO(author, _author)
#define MODULE_DESCRIPTION(_description)	MODULE_INFO(description, _description)
#define MODULE_DEVICE_TABLE(_table, _id)
#define MODULE_FIRMWARE(name)
#define MODULE_VERSION(version)
#define SET_MODULE_OWNER(owner)
#define __MODULE_STRING(str)

#define module_init(fn)
#define module_exit(fn)

#define EXPORT_SYMBOL_GPL(x)

struct module {
	/* Unique handle for this module */
	char name[MODULE_NAME_LEN];
};


#endif /* __LINUX_MODULE_H__ */
