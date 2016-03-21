#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <elf.h>
#include "file.h"
#include "pnkc.h"
#include "page.h"
#include "symbols.h"
#include "gmalloc.h"
#include "module.h"

#define MODULE_ALIGNMENT	128
#define SECTION_ALIGNMENT	128
#define MAX_SECTION_COUNT	16
#define MAX_NAME_SIZE		16
#define ALIGN(addr, align)	(((uintptr_t)(addr) + (align) - 1) & ~((align) - 1))

int module_count;
void* modules[MAX_MODULE_COUNT];

// TODO: Check not to exceed 2~4MB
// TODO: Data to separated area
void module_init() {
#define MAX(a,b) ((a) > (b) ? (a) : (b))
	PNKC* pnkc = (PNKC*)(0x200200 - sizeof(PNKC));
	
	void* addr = (void*)0x200000 + (((uintptr_t)pnkc->smap_offset + (uintptr_t)pnkc->smap_size + 7) & ~7);
	addr += *(uint32_t*)addr;
	addr = (void*)PHYSICAL_TO_VIRTUAL((uintptr_t)addr);
	
	void* buffer = bmalloc(); // 2MB buffer in memory
	if(!buffer) 
		return;
	
	// Open root directory
	int fd = opendir("/boot");
	if(fd < 0) 
		return;
	
	int len;
	Dirent* dirent = gmalloc(sizeof(Dirent));

	while(readdir(fd, dirent) > 0) {
		if((size_t)(strstr((const char*)dirent->name, ".ko") + 3 - (char*)dirent->name) 
				== strlen((const char*)dirent->name)) {
			// Attach root directory for full path
			char file_name[FILE_MAX_NAME_LEN];
			strcpy(file_name, "/boot/");
			strcpy(&file_name[6], dirent->name);
			
			int fd = open(file_name, "r");
			if(fd < 0) {
				printf("\tModule cannot open: %s\n", dirent->name);
				continue;
			} else {
				if((len = read(fd, buffer, 0x200000)) > 0) {
					printf("\tModule loading: %s (%d bytes)\n", dirent->name, len);
					void* base = module_load(buffer, &addr);
					if(base) {
						modules[module_count++] = base;
					} else {
						printf("\t\tFAILED: %s, errno: 0x%x.\n", dirent->name, errno);
					}
				} else {
					printf("\tModule read error: %s\n", dirent->name);
					continue;
				}
			}

		}
	}

	gfree(dirent);
	closedir(fd);
	bfree(buffer);
}

static bool check_header(Elf64_Ehdr* ehdr) {
	if(memcmp(ehdr->e_ident, ELFMAG, 4) != 0) {
		errno = 0x11;	// Illegal ELF64 magic
		return false;
	}

	if(ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		errno = 0x12;	// Not supported ELF type
		return false;
	}

	if(ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		errno = 0x13;	// Not supported ELF data
		return false;
	}

	if(ehdr->e_ident[EI_VERSION] != 1) {
		errno = 0x14;	// Not supported ELF version
		return false;
	}

	if(ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV) {
		errno = 0x15;	// Not supported ELF O/S ABI
		return false;
	}

	if(ehdr->e_ident[EI_ABIVERSION] != 0) {
		errno = 0x16;	// Not supported ELF ABI version
		return false;
	}

	if(ehdr->e_type != ET_REL) {
		errno = 0x17;	// Illegal ELF type
		return false;
	}

	return true;
}

void* module_load(void* file, void **addr) {
	// Check header
	Elf64_Ehdr* ehdr = (Elf64_Ehdr*)file;
	if(!check_header(ehdr))
		return NULL;

	// Find string table
	Elf64_Shdr* shdr = (Elf64_Shdr*)((void*)ehdr + ehdr->e_shoff);
	char* shstrtab = (void*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
	Elf64_Sym* symtab = NULL;
	char* strtab = NULL;

	char* shstr(Elf64_Word offset) {
		return shstrtab + offset;
	}

	char* str(Elf64_Word offset) {
		return strtab + offset;
	}

	// Find symbolic, string table
	for(uint16_t i = 0; i < ehdr->e_shnum; i++) {
		char* name = shstr(shdr[i].sh_name);
		if(shdr[i].sh_type == SHT_SYMTAB && strcmp(name, ".symtab") == 0) {
			symtab = (void*)ehdr + shdr[i].sh_offset;
		} else if(shdr[i].sh_type == SHT_STRTAB && strcmp(name, ".strtab") == 0) {
			strtab = (void*)ehdr + shdr[i].sh_offset;
		}
	}

	if(!symtab || !strtab) {
		errno = 0x42;	// There is no symbolic or string table
		return NULL;
	}

	void* load() {
		// Allocate section area
		void* base = (void*)ALIGN(*addr, MODULE_ALIGNMENT);
		memcpy(base, ehdr, sizeof(Elf64_Ehdr));
		*addr = base + sizeof(Elf64_Ehdr);

		Elf64_Shdr* shdr2 = *addr;
		memcpy(*addr, shdr, sizeof(Elf64_Shdr) * ehdr->e_shnum);
		*addr += sizeof(Elf64_Shdr) * ehdr->e_shnum;

		for(uint16_t i = 0; i < ehdr->e_shnum; i++) {
			char* name = shstr(shdr[i].sh_name);

			if(strstr(name, ".text") == name ||
					strstr(name, ".data") == name ||
					strstr(name, ".bss") == name ||
					strstr(name, ".rodata") == name ||
					strstr(name, ".shstrtab") == name ||
					strstr(name, ".strtab") == name ||
					strstr(name, ".symtab") == name) {

				int align = shdr[i].sh_addralign > SECTION_ALIGNMENT ? shdr[i].sh_addralign : SECTION_ALIGNMENT;
				Elf64_Addr sec_addr = ALIGN(*addr, align);
				Elf64_Xword sec_size = shdr[i].sh_size;
				
				shdr[i].sh_addr = shdr2[i].sh_addr = sec_addr;
				*addr = (void*)sec_addr + sec_size;
				
				printf("\t\tLoading %s to 0x%p (%d bytes)\n", name, sec_addr, sec_size);
				if(strstr(name, ".bss") == name) {
					bzero((void*)sec_addr, sec_size);
				} else {
					memcpy((void*)sec_addr, (void*)ehdr + shdr[i].sh_offset , sec_size);
				}
			}
		}
		
		return base;
	}
	
	// Load sections
	void* base = load();
	if(!base)
		return NULL;

	void* get_section(char* name) {
		for(uint16_t i = 0; i < ehdr->e_shnum; i++) {
			if(strcmp(shstr(shdr[i].sh_name), name) == 0)
				return (void*)shdr[i].sh_addr;
		}

		return NULL;
	}

	bool relocate(Elf64_Rela* rela, int rela_size, void* target) {
		for(int i = 0; i < rela_size; i++) {
			Elf64_Sym* sym = &symtab[rela[i].r_info >> 32];

			int64_t A = rela[i].r_addend;
			uint64_t S;
			if(sym->st_shndx == 0) {
				Symbol* symbol = symbols_get(str(sym->st_name));
				if(!symbol) {
					printf("Cannot find symbol[%d]: %s\n", i, str(sym->st_name));
					errno = 0x31;
					return false;
				}
				S = (uint64_t)symbol->address + sym->st_value;
			} else {
				//S = (uint64_t)(void*)ehdr + shdr[sym->st_shndx].sh_offset + sym->st_value;
				S = (uint64_t)shdr[sym->st_shndx].sh_addr + sym->st_value;
			}
			void* P = target + rela[i].r_offset;

			uint32_t type = rela[i].r_info & 0xffffffff;
			switch(type) {
				case R_X86_64_NONE:
					// Do nothing
					break;
				case R_X86_64_64:
					*(uint64_t*)P = S + A;
					break;
				case R_X86_64_PC32:
					*(uint32_t*)P = S + A - (uint64_t)P;
					break;
				case R_X86_64_COPY:
					// Do nothing
					break;
				case R_X86_64_RELATIVE:
					*(uint64_t*)P = (uint64_t)target + A;
					break;
				case R_X86_64_32:
				case R_X86_64_32S:
					*(uint32_t*)P = S + A;
					break;
				case R_X86_64_16:
					*(uint16_t*)P = S + A;
					break;
				case R_X86_64_PC16:
					*(uint16_t*)P = S + A - (uint64_t)P;
					break;
				case R_X86_64_8:
					*(uint8_t*)P = S + A;
					break;
				case R_X86_64_PC8:
					*(uint8_t*)P = S + A - (uint64_t)P;
					break;
				case R_X86_64_PC64:
					*(uint64_t*)P = S + A - (uint64_t)P;
					break;
				case R_X86_64_SIZE32:
					*(uint32_t*)P = sym->st_size + A;
					break;
				case R_X86_64_SIZE64:
					*(uint64_t*)P = sym->st_size + A;
					break;
				default:
					errno = 0x32;	// Not implemented relocation type
					return false;
			}
		}

		return true;
	}

	// Relocate sections
	for(uint16_t i = 0; i < ehdr->e_shnum; i++) {
		char* name = shstr(shdr[i].sh_name);
		if(strstr(name, ".rela.") == name) {
			void* target = get_section(name + 5);
			if(!target)
				continue;

			printf("\t\tRelocating %s\n", name + 5);

			Elf64_Rela* rela = (void*)ehdr + shdr[i].sh_offset;
			int rela_size = shdr[i].sh_size / sizeof(Elf64_Rela);

			if(!relocate(rela, rela_size, target))
				return NULL;
		}
	}

	return base;
}

void* module_find(void* base, char* name, int type) {
	switch(type) {
		case MODULE_TYPE_NONE:
			type = 0;
			break;
		case MODULE_TYPE_FUNC:
			type = STT_FUNC;
			break;
		case MODULE_TYPE_DATA:
			type = STT_OBJECT;
			break;
		default:
			return NULL;
	}

	Elf64_Ehdr* ehdr = (Elf64_Ehdr*)base;
	Elf64_Shdr* shdr = (Elf64_Shdr*)(base + sizeof(Elf64_Ehdr));
	char* shstrtab = (char*)shdr[ehdr->e_shstrndx].sh_addr;
	char* strtab = NULL;

	Elf64_Sym* symtab = NULL;
	int symtab_size = 0;

	char* shstr(Elf64_Word offset) {
		return shstrtab + offset;
	}

	char* str(Elf64_Word offset) {
		return strtab + offset;
	}

	for(uint16_t i = ehdr->e_shnum - 1; i >= 1; i--) {
		if(shdr[i].sh_type == SHT_SYMTAB) {
			symtab = (Elf64_Sym*)shdr[i].sh_addr;
			symtab_size = shdr[i].sh_size / sizeof(Elf64_Sym);
		} else if(shdr[i].sh_type == SHT_STRTAB && strcmp(shstr(shdr[i].sh_name), ".strtab") == 0) {
			strtab = (char*)shdr[i].sh_addr;
		} else {
			continue;
		}

		if(symtab && strtab)
			break;
	}

	for(int i = 1; i < symtab_size; i++) {
		int bind = ELF64_ST_BIND(symtab[i].st_info);
		int symtype = ELF64_ST_TYPE(symtab[i].st_info);

		if((bind == STB_GLOBAL || bind == STB_WEAK) && (type == 0 || symtype == type) && strcmp(name, str(symtab[i].st_name)) == 0) {
			return (void*)shdr[symtab[i].st_shndx].sh_addr + symtab[i].st_value;
		}
	}

	return NULL;
}

bool module_iface_init(void* addr, void* iface, ModuleObject* objs, int size) {
	for(int i = 0; i < size; i++) {
		void* obj = module_find(addr, objs[i].name, objs[i].type);
		if(objs[i].is_mandatory && !obj)
			return false;

		*(uint64_t*)(iface+ objs[i].offset) = (uint64_t)obj;
	}

	return true;
}
