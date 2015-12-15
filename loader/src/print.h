#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdint.h>

#define NORMAL	0x07
#define PASS	0x02
#define FAIL	(0x04 | 0x08)
#define HEX(v)  (((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')

extern char* video;
extern int print_attr;

void print_init();
void print(const char* str);
void print_32(uint32_t v);
void print_64(uint64_t v);
void tab(int col);
void cursor();
void dump(char* title, uint32_t addr, int size);

#endif /* __PRINT_H__ */

