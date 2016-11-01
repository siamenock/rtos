#ifndef __ELF_H__
#define __ELF_H__

int elf_load(void);
unsigned long elf_get_symbol(char* name);
#endif /*__ELF_H__*/
