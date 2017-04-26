#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>

extern volatile size_t __stdin_head;
extern volatile size_t __stdin_tail;
extern size_t __stdin_size;

extern char __stdout[];
extern volatile size_t __stdout_head;
extern volatile size_t __stdout_tail;
extern size_t __stdout_size;

extern char __stderr[];
extern volatile size_t __stderr_head;
extern volatile size_t __stderr_tail;
extern size_t __stderr_size;

void stdio_init0();

#endif /* __STDIO_H__ */
