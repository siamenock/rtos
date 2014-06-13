#include <util/list.h>
#include <malloc.h>
#include <string.h>
#include "rootfs.h"
#include "symbols.h"

static Symbol* symbols;

void symbols_init() {
	uint32_t size;
	void* mmap = rootfs_file("kernel.smap", &size);
	
	if(mmap) {
		symbols = malloc(size);
		memcpy(symbols, mmap, size);
		
		// Reallocate
		Symbol* s = symbols;
		while(s->name && s->address) {
			s->name += (uint64_t)symbols;
			s++;
		}
	} else {
		printf("\tKernel symbol map not found.\n");
	}

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
