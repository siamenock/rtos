#ifndef __RE_MALLOC_H__
#define __RE_MALLOC_H__

#include <stddef.h>

/**
 * @file re_malloc is a memory allocator which is not using absolute memory address but relative.
 * re_malloc can be used b/w two processors which virtual address is differently mapped.
 * For example, one process's memory allocation pool is mapped to 0x100000 and
 * the other process's memory allocation pool is mapped to 0x200000,
 * both processes can allocate or free memory chunk from the pool.
 * re_malloc uses relative address only but beware if you store pointer to the memory chunk
 * which allocated from re_malloc, the pointer must be translated properly.
 */

/**
 * Initialize the re_malloc pool.
 *
 * @param pool bulk of memory to be allocated
 * @param pool_size size of pool in bytes
 *
 * @return total available size to allocate
 */
size_t re_init(void* pool, size_t pool_size);

/**
 * Allocate memory from re_malloc pool.
 *
 * @param size size of memory chunk to be allocated in bytes
 * @param re_malloc pool
 *
 * @return allocated memory chunk
 */
void* re_malloc(size_t size, void* pool);

/**
 * Restore the memory chunk to re_malloc pool.
 *
 * @param ptr memory chunk to be freed
 * @param pool re_malloc pool
 */
void re_free(void *ptr, void* pool);

/**
 * Resize the memory chunk. The ptr address can be changed if the memory chunk cannot be enlarged directly.
 * If there is no more memory to be enlarged, NULL is returned.
 *
 * @param ptr memory chunk to be enlarged or reduced
 * @param new_size new memory chunk size
 * @param pool re_malloc pool
 *
 * @return resized memory chunk address or NULL if there is no more memory to be allocated
 */
void* re_realloc(void *ptr, size_t new_size, void* pool);

/**
 * Allocate zero initialized array of memory.
 *
 * @param nelem number of elements of array
 * @param elem_size size of a element
 * @param pool re_malloc pool
 *
 * @return array of memory zero initialized
 */
void* re_calloc(size_t nelem, size_t elem_size, void* pool);

/**
 * Total memory can be allocated including memory chunk metadata.
 *
 * @param pool re_malloc pool
 *
 * @return total size of memory can be allocated
 */
size_t re_total(void* pool);

/**
 * Used memory size including memory chunk metadata.
 *
 * @param pool re_malloc pool
 *
 * @return used memory size
 */
size_t re_used(void* pool);

/**
 * Available memory size including memory chunk metadata.
 *
 * @param pool re_malloc pool
 *
 * @return available memory size
 */
size_t re_available(void* pool);

#endif /* __RE_MALLOC_H__ */
