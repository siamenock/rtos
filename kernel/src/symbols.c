#include <string.h>
#include <malloc.h>
#include <util/list.h>
#include "rootfs.h"
#include "symbols.h"

static Symbol* symbols;
//static Map* symbols2;

void symbols_init() {
	uint32_t size;
	void* mmap = rootfs_file("kernel.smap", &size);
	
	if(mmap) {
		symbols = malloc(size);
		memcpy(symbols, mmap, size);
		
		// Relocate
		Symbol* s = symbols;
		while(s->name && s->address) {
			s->name += (uint64_t)symbols;
			s++;
		}
	} else {
		printf("\tKernel symbol map not found.\n");
	}
	
	//symbols2 = map_create(128, map_string_hash, map_string_equals, NULL);
}

Symbol* symbols_get(char* name) {
	if(!symbols)
		return NULL;
	
	Symbol* s = symbols;
	while(s->name && s->address) {
		if(strcmp(s->name, name) == 0)
			return s;
		
		s++;
	}
	
	return NULL;
}
