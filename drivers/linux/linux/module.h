#ifndef __LINUX_MODULE_H__
#define __LINUX_MODULE_H__

#include <linux/string.h>
#include <linux/time.h>

struct module {
};

struct module __this_module;
#define THIS_MODULE	(&__this_module)

#define MODULE_INFO(tag, info)

#define MODULE_LICENSE(_license) MODULE_INFO(license, _license)
#define MODULE_AUTHOR(_author) MODULE_INFO(author, _author)
#define MODULE_DESCRIPTION(_description) MODULE_INFO(description, _description)
#define MODULE_DEVICE_TABLE(_table, _id)

#define module_init(fn)
#define module_exit(fn)

#endif /* __LINUX_MODULE_H__ */
