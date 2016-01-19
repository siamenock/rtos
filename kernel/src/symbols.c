#include <string.h>
#include "pnkc.h"
#include "symbols.h"

static Symbol* symbols;

void symbols_init() {
	PNKC* pnkc = (PNKC*)(0x200200 - sizeof(PNKC));
	symbols =  (void*)0x200000 + pnkc->smap_offset;
	void* end = (void*)0x200000 + pnkc->smap_offset + pnkc->smap_size;
	
	// Relocate
	Symbol* s = symbols;
	while(s->name && s->address && (void*)s < end) {
		s->name += (uintptr_t)symbols;
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
