#include <string.h>
#include "pnkc.h"
#include "symbols.h"

static Symbol* symbols;

void symbols_init() {
	PNKC* pnkc = (PNKC*)(0x200000 - sizeof(PNKC));
	symbols =  (void*)0x200000 + pnkc->smap_offset;
	void* end = (void*)0x200000 + pnkc->smap_offset + pnkc->smap_size;
	
	// Relocate
	Symbol* s = symbols;
	while(s->name && (void*)s < end) {
		s->name += (uintptr_t)symbols;
		s++;
	}
}

Symbol* symbols_get(char* name) {
	if(!symbols)
		return NULL;
	
	PNKC* pnkc = (PNKC*)(0x200000 - sizeof(PNKC));
	void* end = (void*)0x200000 + pnkc->smap_offset + pnkc->smap_size;
	for(Symbol* s = symbols; s->name && (void*)s < end; s++) {
		if(!s->address) continue;

		if(strcmp(s->name, name) == 0) {
			return s;
		}
	}
	
	return NULL;
}
