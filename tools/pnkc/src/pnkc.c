#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include "../../../kernel/src/pnkc.h"

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: pnkc [kernel elf] [kernel pnkc]\n");
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
	
	uint32_t get_offset(char* name, uint64_t base) {
		Elf64_Shdr* hdr = get_shdr(name);
		if(hdr) {
			return hdr->sh_addr - base;
		} else {
			return 0;
		}
	}
	
	uint32_t get_size(char* name) {
		Elf64_Shdr* hdr = get_shdr(name);
		if(hdr) {
			return hdr->sh_size;
		} else {
			return 0;
		}
	}
	
	// Write header
	int fd2 = open(argv[2], O_RDWR | O_CREAT, 0644);
	if(fd2 < 0) {
		printf("Cannot open file: %s\n", argv[2]);
		return 1;
	}
	
	PNKC pnkc;
	pnkc.magic = PNKC_MAGIC;
	pnkc.text_offset = get_offset(".text", 0xffffffff80200000);
	pnkc.text_size = get_size(".text");
	pnkc.rodata_offset = get_offset(".rodata", 0xffffffff80200000);
	pnkc.rodata_size = get_size(".rodata");
	pnkc.data_offset = get_offset(".data", 0xffffffff80400000);
	pnkc.data_size = get_size(".data");
	pnkc.bss_offset = get_offset(".bss", 0xffffffff80400000);
	pnkc.bss_size = get_size(".bss");
	
	// Write PNKC header
	write(fd2, &pnkc, sizeof(PNKC));
	
	void write_body(char* name) {
		Elf64_Shdr* hdr = get_shdr(name);
		if(hdr) {
			write(fd2, (uint8_t*)ehdr + hdr->sh_offset, hdr->sh_size);
		}
	}
	
	write_body(".text");
	write_body(".rodata");
	write_body(".data");
	
	close(fd2);
	close(fd);
	
	return 0;
}
