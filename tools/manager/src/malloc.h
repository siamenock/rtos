#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <stdint.h>
#include <sys/types.h>

void malloc_init();
size_t malloc_total();
size_t malloc_used();

#endif /* __MALLOC_H__ */
