#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "symbols.h"
#include "pnkc.h"
#include "page.h"

Elf64_Ehdr* ehdr;
Elf64_Shdr* strtab;
static char* shstrtab;
Elf64_Shdr* symtab;

// int elf_load(char* elf_file) {
// 	int fd = open(elf_file, O_RDONLY);
// 	if(fd < 0) {
// 		printf("Cannot open file: %s\n", elf_file);
// 		return -1;
// 	}
// 
// 	struct stat state;
// 	if(stat(elf_file, &state) != 0) {
// 		printf("Cannot get state of file: %s\n", elf_file);
// 		return -2;
// 	}
// 
// 	ehdr = mmap(NULL, state.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
// 	if(ehdr == (void*)-1) {
// 		printf("Cannot open file: %s\n", elf_file);
// 		return -3;
// 	}
// 
// 	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
// 		printf("Illegal file format: %s\n", elf_file);
// 		return -4;
// 	}
// 
// 	Elf64_Shdr* shdr = (Elf64_Shdr*)((uint8_t*)ehdr + ehdr->e_shoff);
// 	shstrtab = (char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
// 	Elf64_Shdr* get_shdr(char* name) {
// 		for(int i = 0; i < ehdr->e_shnum; i++) {
// 			if(strcmp(name, shstrtab + shdr[i].sh_name) == 0)
// 				return &shdr[i];
// 		}
// 		return NULL;
// 	}
// 
// 	strtab = get_shdr(".strtab");
// 	if(!strtab)
// 		return -5;
// 
// 	symtab = get_shdr(".symtab");
// 	if(!symtab)
// 		return -6;
// 
// 	return 0;
// }

void copy(void* dst, void* src, size_t size, uint16_t apic_id) {
	memcpy(dst, src, size);
}

void clean(void* dst, size_t size, uint16_t apic_id) {
	memset(dst, 0, size);
}

//bool copy_kernel(Elf64_Ehdr* ehdr) {
bool copy_kernel(PNKC* pnkc) {
// 	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
// 		return false;
// 	}
// 
// 	Elf64_Shdr* shdr = (Elf64_Shdr*)((uint8_t*)ehdr + ehdr->e_shoff);
// 	char* strs = (char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
// 
// 	int t;
// 	for(int i = 0; i < ehdr->e_shnum; i++) {
// 		if(!shdr[i].sh_addr)
// 			continue;
// 
// 		if(!strcmp(strs + shdr[i].sh_name, ".bss")) {
// 			continue;
// 		}
// 
// 		memcpy((void*)VIRTUAL_TO_PHYSICAL(shdr[i].sh_addr), (uint8_t*)ehdr + shdr[i].sh_offset, shdr[i].sh_size);
// 	}
 	uint16_t apic_id = 0;
	void* pos = pnkc;
	pos += sizeof(PNKC);

	if(pnkc->magic != PNKC_MAGIC)
		return false;

	// Copy .text
	copy((void*)0x200000 + pnkc->text_offset, pos, pnkc->text_size, apic_id);
	pos += pnkc->text_size;

	// Copy .rodata
	copy((void*)0x200000 + pnkc->rodata_offset, pos, pnkc->rodata_size, apic_id);
	pos += pnkc->rodata_size;

	// Copy smap
	copy((void*)0x200000 + pnkc->smap_offset, pos, pnkc->smap_size, apic_id);
	pos += pnkc->smap_size;

	// Copy .data
	clean((void*)0x600000, 0x200000, apic_id);
	copy((void*)0x400000 + pnkc->data_offset, pos, pnkc->data_size, apic_id);
	pos += pnkc->data_size;

	// Write PNKC
	copy((void*)(0x200000 /* Kernel entry end */ - sizeof(PNKC)), pnkc, sizeof(PNKC), apic_id);

	return true;
}

int elf_copy(char* elf_file, unsigned long kernel_start_address) {
	int fd = open(elf_file, O_RDONLY);
	if(fd < 0) {
		printf("Cannot open file: %s\n", elf_file);
		return -1;
	}

	off_t elf_size = lseek(fd, 0, SEEK_END);
	char* addr = mmap(NULL, elf_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(addr == MAP_FAILED) {
		close(fd);
		return -2;
	}

	memset((void*)0x200000, 0, 0x600000);

	copy_kernel((PNKC*)addr);
	munmap(addr, elf_size);
	close(fd);

	return 0;
}

uint64_t elf_get_symbol(char* sym_name) {
// 	char* name(int offset) {
// 		return (char*)ehdr + strtab->sh_offset + offset;
// 	}
// 
// 	Elf64_Sym* sym = (void*)ehdr + symtab->sh_offset;
// 	int size = symtab->sh_size / sizeof(Elf64_Sym);
// 	for(int i = 0; i < size; i++) {
// 		if(ELF64_ST_BIND(sym[i].st_info) != STB_GLOBAL)
// 			continue;
// 
// 		if(strlen(sym_name) != strlen(name(sym[i].st_name)))
// 			continue;
// 
// 		if(!strncmp(sym_name, name(sym[i].st_name), strlen(sym_name))) {
// 			return sym[i].st_value;
// 		}
// 		char* test = name(sym[i].st_name);
// 	}
	Symbol* symbol = symbols_get(sym_name);
	if(symbol)
		return (uint64_t)symbol->address;

	return 0;
}

