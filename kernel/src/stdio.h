#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>
#include <stdbool.h>
#include <util/types.h>

#define STDIO_NORMAL  0x07
#define STDIO_PASS    0x02
#define STDIO_FAIL    (0x04 | 0x08)

extern volatile size_t __stdin_head;
extern volatile size_t __stdin_tail;
extern size_t __stdin_size;

void stdio_init(uint8_t apic_id, void* buffer, size_t size);

void stdio_print(const char* str, int row, int col);
void stdio_print_32(uint32_t v, int row, int col);
void stdio_print_64(uint64_t v, int row, int col);

int stdio_putchar(const char ch);
void stdio_scancode(int code);
int stdio_getchar();

#endif /* __STDIO_H__ */
