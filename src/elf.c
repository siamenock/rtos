#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define	ELF_FILE	"kernel.elf"

Elf64_Ehdr* ehdr;
Elf64_Shdr* strtab;
static char* shstrtab;
Elf64_Shdr* symtab;

int elf_load(void) {
	int fd = open(ELF_FILE, O_RDONLY);
	if(fd < 0) {
		printf("Cannot open file: %s\n", ELF_FILE);
		return -1;
	}
	
	struct stat state;
	if(stat(ELF_FILE, &state) != 0) {
		printf("Cannot get state of file: %s\n", ELF_FILE);
		return -2;
	}
	
	ehdr = mmap(NULL, state.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(ehdr == (void*)-1) {
		printf("Cannot open file: %s\n", ELF_FILE);
		return -3;
	}
	
	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
		printf("Illegal file format: %s\n", ELF_FILE);
		return -4;
	}
	
	Elf64_Shdr* shdr = (Elf64_Shdr*)((uint8_t*)ehdr + ehdr->e_shoff);
	shstrtab = (char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
	Elf64_Shdr* get_shdr(char* name) {
		for(int i = 0; i < ehdr->e_shnum; i++) {
			if(strcmp(name, shstrtab + shdr[i].sh_name) == 0)
				return &shdr[i];
		}
		return NULL;
	}
	
	strtab = get_shdr(".strtab");
	if(!strtab)
		return -5;

	symtab = get_shdr(".symtab");
	if(!symtab)
		return -6;

	return 0;
}

unsigned long elf_get_symbol(char* sym_name) {
	char* name(int offset) {
		return (char*)ehdr + strtab->sh_offset + offset;
	}

	Elf64_Sym* sym = (void*)ehdr + symtab->sh_offset;
	int size = symtab->sh_size / sizeof(Elf64_Sym);
	for(int i = 0; i < size; i++) {
		if(ELF64_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;
		
		if(!strncmp(sym_name, name(sym[i].st_name), strlen(sym_name))) {
			return sym[i].st_value;
		}
		char* test = name(sym[i].st_name);
	}
	return 0;
}
