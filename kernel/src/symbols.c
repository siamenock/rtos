#include <string.h>
#include <malloc.h>
#include <util/list.h>
#include "driver/fs.h"
#include "driver/file.h"
#include "symbols.h"

static Symbol* symbols;
//static Map* symbols2;

void symbols_init() {
	int fd = open("/kernel.smap", "r");
	if(fd < 0) {
		printf("\tKernel symbol map not found.\n");
		return ;
	}

	Stat state;
	if(stat("kernel.smap", &state) != 0) {
		printf("\tCannot get state of file: %s\n", "kernel.smap");
		return ;
	}
		
	uint32_t size = state.size;
	uint8_t buf[size];

	if(read(fd, buf, size) != size) {
		printf("Read failed\n");
		close(fd);
		return ;
	}

	close(fd);

	symbols = malloc(size);
	memcpy(symbols, buf, size);

	// Relocate
	Symbol* s = symbols;
	while(s->name && s->address) {
		s->name += (uint64_t)symbols;
		s++;
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
