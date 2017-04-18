#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "smap.h"

int mapping_physical(SMAP* smap);
int mapping_reserved();
int mapping_memory();
int mapping_parse_mem(char* str);

#endif /* __MAPPING_H__ */
