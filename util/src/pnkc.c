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
#include "../../kernel/src/pnkc.h"

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: pnkc [kernel elf] [kernel pnkc] [[module list...]]\n");
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
	
	uint32_t size = 40;
	Elf64_Shdr* hdr = get_shdr(".text");
	if(hdr)
		size += hdr->sh_size;
	
	hdr = get_shdr(".rodata");
	if(hdr)
		size += hdr->sh_size;
	
	hdr = get_shdr(".data");
	if(hdr)
		size += hdr->sh_size;
	
	// Write
	int fd2 = open(argv[2], O_RDWR | O_CREAT, 0644);
	if(fd2 < 0) {
		printf("Cannot open file: %s\n", argv[2]);
		return 1;
	}
	
	// Write PNKC header
	uint8_t header[] = { 0x0f, 0x0d, 0x0a, 0x02, 'P', 'N', 'K', 'C' };
	write(fd2, header, 8);
	
	void write_header(char* name, uint64_t base) {
		Elf64_Shdr* hdr = get_shdr(name);
		if(hdr) {
			uint32_t value = hdr->sh_addr - base;
			write(fd2, &value, 4);
			
			value = hdr->sh_size;
			write(fd2, &value, 4);
		} else {
			uint32_t value = 0;
			write(fd2, &value, 4);
			write(fd2, &value, 4);
		}
	}
	
	void write_body(char* name) {
		printf("\t0x%08x: %s\n", lseek(fd2, 0, SEEK_CUR), name);
		Elf64_Shdr* hdr = get_shdr(name);
		if(hdr) {
			write(fd2, (uint8_t*)ehdr + hdr->sh_offset, hdr->sh_size);
		}
	}
	
	void write_u32(uint32_t count) {
		write(fd2, &count, 4);
	}
	
	bool write_file(char* name) {
		static uint8_t buf[4096];
		
		int fd = open(name, O_RDONLY);
		if(fd < 0)
			return false;
		
		int len = read(fd, buf, 4096);
		while(len > 0) {
			write(fd2, buf, len);
			len = read(fd, buf, 4096);
		}
		close(fd);
		
		return true;
	}
	
	bool ends(char* str, char* token) {
		size_t str_len = strlen(str);
		size_t token_len = strlen(token);
		
		if(str_len < token_len)
			return false;
		
		return strncmp(str + str_len - token_len, token, token_len) == 0;
	}
	
	// Write kernel header
	write_header(".text", 0xffffffff80200000);
	write_header(".rodata", 0xffffffff80200000);
	write_header(".data", 0xffffffff80400000);
	write_header(".bss", 0xffffffff80400000);
	
	// Write file list
	write_u32(argc - 3);
	for(int i = 3; i < argc; i++) {
		if(stat(argv[i], &state) != 0) {
			printf("Cannot get state of file: %s\n", argv[i]);
			return 2;
		}
		
		int type = PNKC_TYPE_UNKNOWN;
		if(ends(argv[i], ".smap")) {
			type = PNKC_TYPE_SYMBOL_MAP;
		} else if(ends(argv[i], ".o") || ends(argv[i], ".ko")) {
			type = PNKC_TYPE_MODULE;
			//printf("Loading module: %s\n", argv[i]);
		}
		
		write_u32(type);	// Module type
		write_u32(state.st_size);
	}
	
	// Write kernel body
	printf("Packing files\n");
	write_body(".text");
	write_body(".rodata");
	write_body(".data");
	close(fd);
	
	// Write files
	for(int i = 3; i < argc; i++) {
		printf("\t0x%08x: %s\n", lseek(fd2, 0, SEEK_CUR), argv[i]);
		if(!write_file(argv[i])) {
			printf("The file cannot read: %s\n", argv[i]);
			return 4;
		}
	}
	
	close(fd2);
	
	return 0;
}
