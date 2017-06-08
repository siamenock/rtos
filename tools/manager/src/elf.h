#ifndef __ELF_H__
#define __ELF_H__

#include <stdint.h>

//int elf_load(char* elf_file);
int elf_copy(char* elf_file, unsigned long kernel_start_address);
uint64_t elf_get_symbol(char* name);

#endif /*__ELF_H__*/
