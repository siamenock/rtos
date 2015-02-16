#ifndef ___MALLOC_H__
#define ___MALLOC_H__

#ifndef size_t
typedef unsigned long size_t;
#endif

size_t __get_used_size(void *mem_pool);
size_t __get_max_size(void* mem_pool);
size_t __get_total_size(void* mem_pool);
void* __malloc(size_t size, void* mem_pool);
void __free(void *ptr, void* mem_pool);
void* __realloc(void *ptr, size_t new_size, void* mem_pool);
void* __calloc(size_t nelem, size_t elem_size, void* mem_pool);

#ifndef DONT_MAKE_WRAPPER

#ifndef get_used_size
#define get_used_size __get_used_size
#endif

#ifndef get_max_size
#define get_max_size __get_used_size
#endif

#ifndef get_total_size
#define get_total_size __get_used_size
#endif

#ifndef malloc
#define malloc __malloc
#endif

#ifndef free
#define free __free
#endif

#ifndef realloc
#define realloc __realloc
#endif

#ifndef calloc
#define calloc __calloc
#endif

#endif /* DONT_MAKE_WRAPPER */

#endif /* ___MALLOC_H__ */
