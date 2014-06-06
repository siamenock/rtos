#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "../../kernel/src/symbols.h"
#include <elf.h>

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: smap [kernel elf] [system map]\n");
		return 0;
	}
	
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0) {
		printf("Cannot open file: %s\n", argv[1]);
		return 1;
	}
	
	struct stat state;
	if(stat(argv[1], &state) != 0) {
		printf("Cannot get state of file: %s\n", argv[1]);
		return 2;
	}
	
	Elf64_Ehdr* ehdr = mmap(NULL, state.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(ehdr == (void*)-1) {
		printf("Cannot open file: %s\n", argv[1]);
		return 1;
	}
	
	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
		printf("Illegal file format: %s\n", argv[1]);
		return 3;
	}
	
	Elf64_Shdr* shdr = (Elf64_Shdr*)((uint8_t*)ehdr + ehdr->e_shoff);
	char* strs = (char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
	Elf64_Shdr* get_shdr(char* name) {
		for(int i = 0; i < ehdr->e_shnum; i++) {
			if(strcmp(name, strs + shdr[i].sh_name) == 0)
				return &shdr[i];
		}
		return NULL;
	}
	
	Elf64_Shdr* strtab = get_shdr(".strtab");
	char* str = (char*)ehdr + strtab->sh_offset;
	char* name(int offset) {
		return str + offset;
	}
	
	int count = 0;
	int text_size = 0;
	
	Elf64_Shdr* symtab = get_shdr(".symtab");
	Elf64_Sym* sym = (void*)ehdr + symtab->sh_offset;
	int size = symtab->sh_size / sizeof(Elf64_Sym);
	for(int i = 0; i < size; i++) {
		if(ELF64_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;
		
		int type = ELF64_ST_TYPE(sym[i].st_info);
		switch(type) {
			case STT_NOTYPE:
			case STT_OBJECT:
			case STT_FUNC:
				break;
			default:
				printf("Unknown symbol type: %d for %s\n", type, name(sym[i].st_name));
				return 4;
		}
		
		count++;
		text_size += strlen(name(sym[i].st_name)) + 1;
	}
	
	// Make data structure
	int total = sizeof(Symbol) * (count + 1) + text_size;
	Symbol* symbols = malloc(total);
	char* text = (void*)symbols + total - text_size;
	int j = 0;
	for(int i = 0; i < size; i++) {
		if(ELF64_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;
		
		int type = ELF64_ST_TYPE(sym[i].st_info);
		switch(type) {
			case STT_NOTYPE:
				type = SYMBOL_TYPE_NONE;
				break;
			case STT_OBJECT:
				type = SYMBOL_TYPE_DATA;
				break;
			case STT_FUNC:
				type = SYMBOL_TYPE_FUNC;
				break;
			default:
				;// Not possible
		}
		
		symbols[j].name = (void*)((void*)text - (void*)symbols);
		symbols[j].type = type;
		symbols[j].address = (void*)(uint64_t)sym[i].st_value;
		j++;
		
		char* symname = name(sym[i].st_name);
		int len = strlen(symname) + 1;
		memcpy(text, symname, len);
		text += len;
	}
	close(fd);
	
	// Write
	int fd2 = open(argv[2], O_RDWR | O_CREAT, 0644);
	if(fd2 < 0) {
		printf("Cannot open file: %s\n", argv[2]);
		return 1;
	}
	
	int len = write(fd2, symbols, total);
	if(len != total) {
		printf("Cannot write fully: %d < %d\n", len, total);
		return 5;
	}
	
	close(fd2);
	
	return 0;
}
