#ifndef __UTIL_CACHE_H__
#define __UTIL_CACHE_H__

#include <util/list.h>
#include <util/map.h>

typedef struct {
	Map*	map;
	List*	list;
	size_t 	capacity;
	void	(*uncache)(void*);
	void*	pool;
} Cache;

Cache* cache_create(size_t capacity, void(*uncache)(void*), void* pool);
void cache_destroy(Cache* cache);
void* cache_get(Cache* cache, void* key);
bool cache_set(Cache* cache, void* key, void* data);
void cache_clear(Cache* cache);

#endif /* __UTIL_CACHE_H__ */

