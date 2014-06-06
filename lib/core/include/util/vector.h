#ifndef __UTIL_VECTOR_H__
#define __UTIL_VECTOR_H__

#include <stddef.h>
#include <stdbool.h>

typedef struct _Vector {
	size_t	index;
	size_t	size;
	void**		array;
	
	void*(*malloc)(size_t);
	void(*free)(void*);
} Vector;

Vector* vector_create(size_t size, void*(*malloc)(size_t), void(*free)(void*));
void vector_destroy(Vector* vector);
void vector_init(Vector* vector, void** array, size_t size);
bool vector_available(Vector* vector);
bool vector_is_empty(Vector* vector);
bool vector_add(Vector* vector, void* data);
void* vector_get(Vector* vector, size_t index);
size_t vector_index_of(Vector* vector, void* data, bool(*comp_fn)(void*,void*));
void* vector_remove(Vector* vector, size_t index);
size_t vector_size(Vector* vector);
size_t vector_capacity(Vector* vector);

typedef struct _VectorIterator {
	Vector* vector;
	size_t index;
} VectorIterator;

void vector_iterator_init(VectorIterator* iter, Vector* vector);
bool vector_iterator_has_next(VectorIterator* iter);
void* vector_iterator_next(VectorIterator* iter);
void* vector_iterator_remove(VectorIterator* iter);

#endif /* __UTIL_VECTOR_H__ */
