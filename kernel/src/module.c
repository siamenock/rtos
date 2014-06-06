#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <malloc.h>
#include <elf.h>
#include "../../loader/src/page.h"
#include "pnkc.h"
#include "symbols.h"
#include "module.h"

#define CODE_ALIGN		16

int module_count;
void* modules[MAX_MODULE_COUNT];

void module_init() {
	#define MAX(a,b) ((a) > (b) ? (a) : (b))
	
	PNKC* pnkc = pnkc_find();
	uint32_t* size = pnkc->text_offset + pnkc->text_size > pnkc->rodata_offset + pnkc->rodata_size ? &pnkc->text_size : &pnkc->rodata_size;
	uint8_t* addr = (void*)PHYSICAL_TO_VIRTUAL(0x200000 + MAX(pnkc->text_offset + pnkc->text_size, pnkc->rodata_offset + pnkc->rodata_size));
	
	uint8_t* org_addr = addr;
	uint32_t count = pnkc->file_count;
	for(uint32_t i = 0; i < count; i++) {
		PNKC_File file;
		pnkc_file_get(&file, i);
		
		if(file.type == PNKC_TYPE_MODULE) {
			printf("\tModule loadeding[%d]: 0x%08x(%d) ", i, (void*)file.address - (void*)pnkc, file.size);
			addr = module_load(file.address, file.size, addr);
			if(addr) {
				modules[module_count++] = addr;
				printf("Done\n");
				
				addr += file.size;
			} else {
				printf("FAILED: %dth file, errno: 0x%x.\n", i, errno);
			}
		}
	}
	
	// Extend kernel code/rodata area
	*size += addr - org_addr;
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

static void* init(void* file, size_t size, void* addr) {
	addr = (void*)(((uint64_t)addr + CODE_ALIGN - 1) & ~(CODE_ALIGN - 1));	// 16 bytes align
	bzero(addr, size);
	memcpy(addr, file, size);
	
	return addr;
}

static bool relocate(void* addr) {
	Elf64_Ehdr* ehdr = (Elf64_Ehdr*)addr;
	Elf64_Shdr* shdr = (Elf64_Shdr*)(addr + ehdr->e_shoff);
	char* shstrtab = addr + shdr[ehdr->e_shstrndx].sh_offset;
	char* strtab = NULL;
	
	char* shstr(Elf64_Word offset) {
		return shstrtab + offset;
	}
	
	char* str(Elf64_Word offset) {
		return strtab + offset;
	}
	
	Elf64_Rela* rela_text = NULL;
	int rela_text_size = 0;
	Elf64_Rel* rel_text = NULL;
	int rel_text_size = 0;
	void* text = NULL;
	
	Elf64_Rela* rela_data = NULL;
	int rela_data_size = 0;
	Elf64_Rel* rel_data = NULL;
	int rel_data_size = 0;
	void* data = NULL;
	
	Elf64_Sym* symtab = NULL;
	
	for(uint16_t i = 0; i < ehdr->e_shnum; i++) {
		char* name = shstr(shdr[i].sh_name);
		if(strcmp(name, ".text") == 0) {
			text = addr + shdr[i].sh_offset;
		} else if(strcmp(name, ".data") == 0) {
			data = addr + shdr[i].sh_offset;
		} else if(strcmp(name, ".rela.text") == 0) {
			rela_text = addr + shdr[i].sh_offset;
			rela_text_size = shdr[i].sh_size / sizeof(Elf64_Rela);
		} else if(strcmp(name, ".rel.text") == 0) {
			rel_text = addr + shdr[i].sh_offset;
			rel_text_size = shdr[i].sh_size / sizeof(Elf64_Rel);
		} else if(strcmp(name, ".rela.data") == 0) {
			rela_data = addr + shdr[i].sh_offset;
			rela_data_size = shdr[i].sh_size / sizeof(Elf64_Rela);
		} else if(strcmp(name, ".rel.data") == 0) {
			rel_data = addr + shdr[i].sh_offset;
			rel_data_size = shdr[i].sh_size / sizeof(Elf64_Rel);
		} else if(strcmp(name, ".symtab") == 0) {
			symtab = addr + shdr[i].sh_offset;
		} else if(shdr[i].sh_type == SHT_STRTAB && strcmp(shstr(shdr[i].sh_name), ".strtab") == 0) {
			strtab = addr + shdr[i].sh_offset;
		}
	}
	
	if(symtab == NULL) {
		errno = 0x33;	// .symtab section not found
		return false;
	} else if(strtab == NULL) {
		errno = 0x34;	// .strtab section not found
		return false;
	}
	
	// Relocate module
	bool relocate_base(Elf64_Rela* rela, int rela_size, Elf64_Rel* rel, int rel_size, void* base) {
		for(int i = 0; i < rela_size; i++) {
			Elf64_Sym* sym = &symtab[rela[i].r_info >> 32];
			
			int64_t A = rela[i].r_addend;
			uint64_t S;
			if(sym->st_shndx == 0) {
				Symbol* symbol = symbols_get(str(sym->st_name));
				if(!symbol) {
					printf("Cannot find symbol[%d]: %s\n", i, str(sym->st_name));
					errno = 0x39;
					return false;
				}
				S = (uint64_t)symbol->address + sym->st_value;
			} else {
				S = (uint64_t)addr + shdr[sym->st_shndx].sh_offset + sym->st_value;
			}
			void* P = base + rela[i].r_offset;
			
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
					*(uint64_t*)P = (uint64_t)base + A;
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
					errno = 0x38;	// Not implemented relocation type
					return false;
			}
		}
		
		for(int i = 0; i < rel_size; i++) {
			uint32_t type = rel[i].r_info & 0xffffffff;
			switch(type) {
				case R_X86_64_NONE:
					// Do nothing
					break;
				case R_X86_64_COPY:
					// Do nothing
					break;
				default:
					errno = 0x38;	// Not implemented relocation type
					return false;
			}
		}
		
		return true;
	}
	
	if((rela_text || rel_text)) {
		if(!text) {
			errno = 0x31;	// .text section not found
			return false;
		}
		
		if(!relocate_base(rela_text, rela_text_size, rel_text, rel_text_size, text))
			return false;
	}
	
	if((rela_data || rel_data)) {
		if(!data) {
			errno = 0x32;	// .data section not found
			return false;
		}
		
		if(!relocate_base(rela_data, rela_data_size, rel_data, rel_data_size, data))
			return false;
	}
	
	return true;
}

void* module_load(void* file, size_t size, void *addr) {
	Elf64_Ehdr* ehdr = (Elf64_Ehdr*)file;
	
	// Check header
	if(!check_header(ehdr))
		return NULL;
	
	// Initilize module area
	addr = init(file, size, addr);
	
	// Relocate
	if(!relocate(addr)) {
		return NULL;
	}
	
	return addr;
}

void* module_find(void* addr, char* name, int type) {
	int type2;
	switch(type) {
		case MODULE_TYPE_NONE:
			type2 = 0;
			break;
		case MODULE_TYPE_FUNC:
			type2 = STT_FUNC;
			break;
		case MODULE_TYPE_DATA:
			type2 = STT_OBJECT;
			break;
		default:
			return NULL;
	}
	
	Elf64_Ehdr* ehdr = (Elf64_Ehdr*)addr;
	Elf64_Shdr* shdr = (Elf64_Shdr*)(addr + ehdr->e_shoff);
	char* shstrtab = addr + shdr[ehdr->e_shstrndx].sh_offset;
	char* strtab = NULL;
	
	char* shstr(Elf64_Word offset) {
		return shstrtab + offset;
	}
	
	char* str(Elf64_Word offset) {
		return strtab + offset;
	}
	
	Elf64_Sym* symtab = NULL;
	int symtab_size = 0;
	for(uint16_t i = 0; i < ehdr->e_shnum; i++) {
		if(shdr[i].sh_type == SHT_SYMTAB) {
			symtab = addr + shdr[i].sh_offset;
			symtab_size = shdr[i].sh_size / sizeof(Elf64_Sym);
		} else if(shdr[i].sh_type == SHT_STRTAB && strcmp(shstr(shdr[i].sh_name), ".strtab") == 0) {
			strtab = addr + shdr[i].sh_offset;
		}
	}
	
	if(symtab == NULL || strtab == NULL)
		return NULL;
	
	for(int i = 1; i < symtab_size; i++) {
		int bind = ELF64_ST_BIND(symtab[i].st_info);
		int symtype = ELF64_ST_TYPE(symtab[i].st_info);
		
		if((bind == STB_GLOBAL || bind == STB_WEAK) && (type2 == 0 || symtype == type2) && strcmp(name, str(symtab[i].st_name)) == 0)
			return addr + shdr[symtab[i].st_shndx].sh_offset + symtab[i].st_value;
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
