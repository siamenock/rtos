#ifndef __UTIL_MAP_H__
#define __UTIL_MAP_H__

#include <stdint.h>
#include "list.h"

typedef struct _MapEntry {
	void*	key;
	void*	data;
} MapEntry;

typedef struct _Map {
	List**		table;
	size_t		threshold;
	size_t		capacity;
	
	size_t		size;
	
	void*(*malloc)(size_t,void*);
	void(*free)(void*,void*);
	void*		pool;
	
	uint64_t(*hash)(void*);
	bool(*equals)(void*,void*);
} Map;

Map* map_create(size_t initial_capacity, uint64_t(*hash)(void*), bool(*equals)(void*,void*), void* malloc, void* free, void* pool);
void map_destroy(Map* map);
bool map_is_empty(Map* map);
bool map_put(Map* map, void* key, void* data);
bool map_update(Map* map, void* key, void* data);
void* map_get(Map* map, void* key);
void* map_get_key(Map* map, void* key);
bool map_contains(Map* map, void* key);
void* map_remove(Map* map, void* key);
size_t map_capacity(Map* map);
size_t map_size(Map* map);

typedef struct _MapIterator {
	Map*		map;
	size_t		index;
	size_t		list_index;
	MapEntry	entry;
} MapIterator;

void map_iterator_init(MapIterator* iter, Map* map);
bool map_iterator_has_next(MapIterator* iter);
MapEntry* map_iterator_next(MapIterator* iter);
MapEntry* map_iterator_remove(MapIterator* iter);

uint64_t map_uint64_hash(void* key);
bool map_uint64_equals(void* key1, void* key2);
uint64_t map_string_hash(void* key);
bool map_string_equals(void* key1, void* key2);

#endif /* __UTIL_MAP_H__ */
