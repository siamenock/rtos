#include <util/list.h>
#include <malloc.h>
#include <string.h>
#include "pnkc.h"
#include "symbols.h"

static Symbol* symbols;

void symbols_init() {
	PNKC_File file;
	
	uint32_t count = pnkc_file_count();
	for(uint32_t i = 0; i < count; i++) {
		pnkc_file_get(&file, i);
		
		if(file.type == PNKC_TYPE_SYMBOL_MAP) {
			goto found;
		}
	}
	
	printf("\tKernel symbol map not found.\n");
	return;

found:
	symbols = malloc(file.size);
	memcpy(symbols, file.address, file.size);
	
	// Reallocate
	Symbol* s = symbols;
	while(s->name && s->address) {
		s->name += (uint64_t)symbols;
		s++;
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
