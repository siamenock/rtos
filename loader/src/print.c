#include <stdint.h>
#include "print.h"

char* video;
int print_attr = NORMAL;

void print_init() {
	uint8_t pos1, pos2;
	
	asm volatile("outb %%al, %%dx" : : "d"(0x3d4), "a"(0x0f));
	asm volatile("inb %%dx, %%al" : "=a"(pos1) : "d"(0x3d5));
	asm volatile("outb %%al, %%dx" : : "d"(0x3d4), "a"(0x0e));
	asm volatile("inb %%dx, %%al" : "=a"(pos2) : "d"(0x3d5));
	
	video = (char*)0xb8000 + 2 * ((uint32_t)pos2 << 8 | (uint32_t)pos1);
}

void print(const char* str) {
	if(!video)
		return;
	
	while(*str != 0) {
		if(*str == '\n') {
			video = (char*)(((uint32_t)((uint32_t)video  - 0xb8000 + 160) / 160) * 160 + 0xb8000);
		} else {
			*video++ = *str;
			*video++ = print_attr;
		}
		str++;
	}
	
	cursor();
}

void print_32(uint32_t v) {
	if(!video)
		return;
	
	*video++ = HEX(v >> 28);
	*video++ = print_attr;
	*video++ = HEX(v >> 24);
	*video++ = print_attr;
	*video++ = HEX(v >> 20);
	*video++ = print_attr;
	*video++ = HEX(v >> 16);
	*video++ = print_attr;
	*video++ = HEX(v >> 12);
	*video++ = print_attr;
	*video++ = HEX(v >> 8);
	*video++ = print_attr;
	*video++ = HEX(v >> 4);
	*video++ = print_attr;
	*video++ = HEX(v >> 0);
	*video++ = print_attr;
	
	cursor();
}

void print_64(uint64_t v) {
	if(!video)
		return;
	
	*video++ = HEX(v >> 60);
	*video++ = print_attr;
	*video++ = HEX(v >> 56);
	*video++ = print_attr;
	*video++ = HEX(v >> 52);
	*video++ = print_attr;
	*video++ = HEX(v >> 48);
	*video++ = print_attr;
	*video++ = HEX(v >> 44);
	*video++ = print_attr;
	*video++ = HEX(v >> 40);
	*video++ = print_attr;
	*video++ = HEX(v >> 36);
	*video++ = print_attr;
	*video++ = HEX(v >> 32);
	*video++ = print_attr;
	
	*video++ = HEX(v >> 28);
	*video++ = print_attr;
	*video++ = HEX(v >> 24);
	*video++ = print_attr;
	*video++ = HEX(v >> 20);
	*video++ = print_attr;
	*video++ = HEX(v >> 16);
	*video++ = print_attr;
	*video++ = HEX(v >> 12);
	*video++ = print_attr;
	*video++ = HEX(v >> 8);
	*video++ = print_attr;
	*video++ = HEX(v >> 4);
	*video++ = print_attr;
	*video++ = HEX(v >> 0);
	*video++ = print_attr;
	
	cursor();
}

void tab(int col) {
	if(!video)
		return;
	
	uint32_t pos = (uint32_t)video - 0xb8000;
	if(pos % 160 < col * 2) {
		video += col * 2 - pos % 160;
	} else {
		video += 160 - pos % 160 + col * 2;
	}
	
	cursor();
}

void cursor() {
	if(!video)
		return;
	
	uint32_t pos =  ((uint32_t)video - 0xb8000) / 2;
	
	asm volatile("outb %%al, %%dx" : : "d"(0x3d4), "a"(0x0f));
	asm volatile("outb %%al, %%dx" : : "d"(0x3d5), "a"(pos & 0xff));
	asm volatile("outb %%al, %%dx" : : "d"(0x3d4), "a"(0x0e));
	asm volatile("outb %%al, %%dx" : : "d"(0x3d5), "a"((pos >> 8) & 0xff));
}

void dump(char* title, uint32_t addr, int size) {
	if(!video)
		return;
	
	print("\n**** ");
	print(title);
	print(" ****\n");
	
	uint8_t* p = (uint8_t*)addr;
	for(int i = 0; i < size; i++) {
		*video++ = HEX(*p >> 4);
		*video++ = print_attr;
		*video++ = HEX(*p >> 0);
		*video++ = print_attr;
		
		if((i + 1) % 4 == 0) {
			*video++ = ' ';
			*video++ = print_attr;
			
			if((i + 1) % 32 == 0) {
				video = (char*)(((uint32_t)((uint32_t)video  - 0xb8000 + 160) / 160) * 160 + 0xb8000);
			}
		}
		
		p++;
	}
	
	cursor();
}
