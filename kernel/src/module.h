#ifndef __MODULE_H__
#define __MODULE_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
	MODULE_TYPE_NONE,
	MODULE_TYPE_FUNC,
	MODULE_TYPE_DATA,
} ModuleType;

typedef struct {
	char*		name;
	int		type;
	int		offset;
	bool		is_mandatory;
} ModuleObject;

#define MAX_MODULE_COUNT	32

extern int module_count;
extern void* modules[MAX_MODULE_COUNT];

void module_init();

void* module_load(void* file, size_t size, void* addr);
void* module_get(int idx);
void* module_find(void* addr, char* name, int type);
bool module_iface_init(void* addr, void* iface, ModuleObject* objs, int size);

#endif /* __MODULE_H__ */
