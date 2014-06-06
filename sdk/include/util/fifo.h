#ifndef __UTIL_FIFO_H__
#define __UTIL_FIFO_H__

#include <stddef.h>
#include <stdbool.h>

typedef struct _FIFO {
	size_t		head;
	size_t		tail;
	size_t		size;
	void**		array;
	void*(*malloc)(size_t);
	void(*free)(void*);
} FIFO;

FIFO* fifo_create(size_t size, void*(*malloc)(size_t), void(*free)(void*));
void fifo_destroy(FIFO* fifo);
bool fifo_resize(FIFO* fifo, size_t size, void(*popped)(void*));
void fifo_init(FIFO* fifo, void** array, size_t size);
void fifo_reinit(FIFO* fifo, void** array, size_t size, void(*popped)(void*));
bool fifo_push(FIFO* fifo, void* data);
void* fifo_pop(FIFO* fifo);
void* fifo_peek(FIFO* fifo, size_t index);
size_t fifo_size(FIFO* fifo);
size_t fifo_capacity(FIFO* fifo);
bool fifo_available(FIFO* fifo);
bool fifo_empty(FIFO* fifo);

#endif /* __UTIL_FIFO_H__ */
