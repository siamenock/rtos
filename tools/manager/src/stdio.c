#include <stdio.h>
#include "elf.h"
#include "page.h"

#define BUFFER_SIZE	4096

char __stdin[BUFFER_SIZE];
volatile size_t __stdin_head;
volatile size_t __stdin_tail;
size_t __stdin_size = BUFFER_SIZE;

char* __stdout[BUFFER_SIZE];
volatile size_t* __stdout_head;
volatile size_t* __stdout_tail;
size_t __stdout_size = BUFFER_SIZE;

char __stderr[BUFFER_SIZE];
volatile size_t __stderr_head;
volatile size_t __stderr_tail;
size_t __stderr_size = BUFFER_SIZE;

void stdio_init0() {
	*__stdout = (char*)VIRTUAL_TO_PHYSICAL(elf_get_symbol("__stdout"));
	__stdout_head = (volatile size_t*)VIRTUAL_TO_PHYSICAL(elf_get_symbol("__stdout_head"));
	__stdout_tail = (volatile size_t*)VIRTUAL_TO_PHYSICAL(elf_get_symbol("__stdout_tail"));

	if(!*__stdout || !__stdout_head || !__stdout_tail)
		printf("Can not find standard out symobols\n");

	printf("\t__stdout: %p\n", *__stdout);
	printf("\t__stdout_head: %p\n", __stdout_head);
	printf("\t__stdout_tail: %p\n", __stdout_tail);
}
