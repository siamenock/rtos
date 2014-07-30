#include <string.h>
#include <tlsf.h>
#include <util/map.h>

// TODO: Change accessing list using index to using iterator.

#define THRESHOLD(cap)	(((cap) >> 2) + ((cap) >> 4))	// 75%

Map* map_create(size_t initial_capacity, uint64_t(*hash)(void*), bool(*equals)(void*,void*), void* pool) {
	extern void* __malloc_pool;
	if(pool == NULL)
		pool = __malloc_pool;
	
	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;
	
	Map* map = malloc_ex(sizeof(Map), pool);
	if(!map)
		return NULL;
	map->table = malloc_ex(sizeof(MapEntry) * capacity, pool);
	if(!map->table) {
		free_ex(map, pool);
		return NULL;
	}
	bzero(map->table, sizeof(MapEntry) * capacity);
	map->capacity = capacity;
	map->threshold = THRESHOLD(capacity);
	map->size = 0;
	map->hash = hash;
	map->equals = equals;
	map->pool = pool;
	
	return map;
}

static void destroy(Map* map) {
	for(size_t i = 0; i < map->capacity; i++) {
		List* list = map->table[i];
		if(!list)
			continue;
		
		ListIterator iter;
		list_iterator_init(&iter, list);
		while(list_iterator_has_next(&iter)) {
			MapEntry* entry = list_iterator_next(&iter);
			free_ex(entry, map->pool);
		}
		
		list_destroy(list);
	}
	
	free_ex(map->table, map->pool);
}

void map_destroy(Map* map) {
	destroy(map);
	free_ex(map, map->pool);
}

bool map_is_empty(Map* map) {
	return map->size == 0;
}

bool map_put(Map* map, void* key, void* data) {
	if(map->size + 1 > map->threshold) {
		// Create new map
		Map map2;
		size_t capacity = map->capacity * 2;
		map2.table = malloc_ex(sizeof(List*) * capacity, map->pool);
		if(!map2.table)
			return false;
		bzero(map2.table, sizeof(List*) * capacity);
		map2.capacity = capacity;
		map2.threshold = THRESHOLD(capacity);
		map2.size = 0;
		map2.hash = map->hash;
		map2.equals = map->equals;
		map2.pool = map->pool;
		
		// Copy
		MapIterator iter;
		map_iterator_init(&iter, map);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			if(!map_put(&map2, entry->key, entry->data)) {
				destroy(&map2);
				return false;
			}
		}
		
		// Destory
		destroy(map);
		
		// Paste
		memcpy(map, &map2, sizeof(Map));
	}
	
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		map->table[index] = list_create(map->pool);
		if(!map->table[index])
			return false;
	} else {
		size_t size = list_size(map->table[index]);
		for(int i = 0; i < size; i++) {
			MapEntry* entry = list_get(map->table[index], i);
			if(map->equals(entry->key, key))
				return false;
		}
	}
	
	MapEntry* entry = malloc_ex(sizeof(MapEntry), map->pool);
	if(!entry)
		return false;
	
	entry->key = key;
	entry->data = data;
	
	if(!list_add(map->table[index], entry)) {
		free_ex(entry, map->pool);
		return false;
	}
	map->size++;
	
	return true;
}

bool map_update(Map* map, void* key, void* data) {
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		return false;
	}
	
	size_t size = list_size(map->table[index]);
	for(int i = 0; i < size; i++) {
		MapEntry* entry = list_get(map->table[index], i);
		if(map->equals(entry->key, key)) {
			entry->data = data;
			return true;
		}
	}
	
	return false;
}

void* map_get(Map* map, void* key) {
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		return NULL;
	}
	
	size_t size = list_size(map->table[index]);
	for(int i = 0; i < size; i++) {
		MapEntry* entry = list_get(map->table[index], i);
		if(map->equals(entry->key, key))
			return entry->data;
	}
	
	return NULL;
}

void* map_get_key(Map* map, void* key) {
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		return NULL;
	}
	
	size_t size = list_size(map->table[index]);
	for(int i = 0; i < size; i++) {
		MapEntry* entry = list_get(map->table[index], i);
		if(map->equals(entry->key, key))
			return entry->key;
	}
	
	return NULL;
}

bool map_contains(Map* map, void* key) {
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		return false;
	}
	
	size_t size = list_size(map->table[index]);
	for(int i = 0; i < size; i++) {
		MapEntry* entry = list_get(map->table[index], i);
		if(map->equals(entry->key, key))
			return true;
	}
	
	return false;
}

void* map_remove(Map* map, void* key) {
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		return NULL;
	}
	
	size_t size = list_size(map->table[index]);
	for(int i = 0; i < size; i++) {
		MapEntry* entry = list_get(map->table[index], i);
		if(map->equals(entry->key, key)) {
			void* data = entry->data;
			list_remove(map->table[index], i);
			free_ex(entry, map->pool);
			
			if(list_is_empty(map->table[index])) {
				list_destroy(map->table[index]);
				map->table[index] = NULL;
			}
			
			map->size--;
			
			return data;
		}
	}
	
	return NULL;
}

size_t map_capacity(Map* map) {
	return map->capacity;
}

size_t map_size(Map* map) {
	return map->size;
}

void map_iterator_init(MapIterator* iter, Map* map) {
	iter->map = map;
	for(iter->index = 0; iter->index < map->capacity && !map->table[iter->index]; iter->index++);
	iter->list_index = 0;
}

bool map_iterator_has_next(MapIterator* iter) {
	if(iter->index >= iter->map->capacity)
		return false;
	
	if(iter->map->table[iter->index] && iter->list_index < list_size(iter->map->table[iter->index]))
		return true;
	
	for(iter->index++; iter->index < iter->map->capacity && !iter->map->table[iter->index]; iter->index++);
	
	if(iter->index < iter->map->capacity) {
		iter->list_index = 0;
		return true;
	} else {
		return false;
	}
}

MapEntry* map_iterator_next(MapIterator* iter) {
	MapEntry* entry = list_get(iter->map->table[iter->index], iter->list_index++);
	iter->entry.key = entry->key;
	iter->entry.data = entry->data;
	
	return &iter->entry;
}

MapEntry* map_iterator_remove(MapIterator* iter) {
	MapEntry* entry = list_remove(iter->map->table[iter->index], iter->list_index - 1);
	iter->entry.key = entry->key;
	iter->entry.data = entry->data;
	free_ex(entry, iter->map->pool);
	
	if(list_is_empty(iter->map->table[iter->index])) {
		list_destroy(iter->map->table[iter->index]);
		iter->map->table[iter->index] = NULL;
	}
	
	iter->map->size--;
	
	return &iter->entry;
}

uint64_t map_uint64_hash(void* key) {
	return (uint64_t)key;
}

bool map_uint64_equals(void* key1, void* key2) {
	return key1 == key2;
}

uint64_t map_string_hash(void* key) {
	char* c = key;
	uint32_t len = 0;
	uint32_t sum = 0;
	while(*c != '\0') {
		len++;
		sum += *c++;
	}
	
	return ((uint64_t)len) << 32 | (uint64_t)sum;
}

bool map_string_equals(void* key1, void* key2) {
	char* c1 = key1;
	char* c2 = key2;
	
	while(*c1 != '\0' && *c2 != '\0') {
		if(*c1++ != *c2++)
			return false;
	}
	
	return true;
}
