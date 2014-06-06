#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>

void stdio_init();
void stdio_init2(void* buf, size_t size);
void stdio_event(void*);

void stdio_scancode(int code);
int stdio_getchar();

#endif /* __STDIO_H__ */
