#include <string.h>
#include <util/map.h>

// TODO: Change accessing list using index to using iterator.

#define THRESHOLD(cap)	(((cap) >> 2) + ((cap) >> 4))	// 75%

Map* map_create(size_t initial_capacity, uint64_t(*hash)(void*), bool(*equals)(void*,void*), void*(*malloc)(size_t), void(*free)(void*)) {
	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;
	
	Map* map = malloc(sizeof(Map));
	if(!map)
		return NULL;
	map->table = malloc(sizeof(MapEntry) * capacity);
	if(!map->table) {
		free(map);
		return NULL;
	}
	bzero(map->table, sizeof(MapEntry) * capacity);
	map->capacity = capacity;
	map->threshold = THRESHOLD(capacity);
	map->size = 0;
	map->malloc = malloc;
	map->free = free;
	map->hash = hash;
	map->equals = equals;
	
	return map;
}

void map_destroy(Map* map) {
	MapIterator iter;
	map_iterator_init(&iter, map);
	while(map_iterator_has_next(&iter)) {
		map->free(map_iterator_next(&iter));
	}
	map->free(map->table);
	map->free(map);
}

bool map_is_empty(Map* map) {
	return map->size == 0;
}

bool map_put(Map* map, void* key, void* data) {
	if(map->size + 1 > map->threshold) {
		size_t capacity2 = map->capacity * 2;
		List** table2 = map->malloc(sizeof(List*) * capacity2);
		if(!table2)
			return false;
		
		bzero(table2, sizeof(List*) * capacity2);

		for(size_t i = 0; i < map->capacity; i++) {
			if(map->table[i]) {
				size_t size = list_size(map->table[i]);
				for(int j = 0; j < size; j++) {
					MapEntry* entry = list_get(map->table[i], j);
					
					size_t index = map->hash(entry->key) % capacity2;
					if(!table2[index]) {
						table2[index] = list_create(map->malloc, map->free);
						
						if(!table2[index])
							goto failed;
					}
					
					if(!list_add(table2[index], entry))
						goto failed;
				}
			}
		}
		
		for(size_t i = 0; i < map->capacity; i++)
			if(map->table[i])
				list_destroy(map->table[i]);
		
		map->free(map->table);
		
		map->table = table2;
		map->capacity = capacity2;
		map->threshold = THRESHOLD(capacity2);
		
		goto succeed;

failed:
		for(size_t i = 0; i < capacity2; i++)
			if(table2[i])
				list_destroy(table2[i]);
		
		map->free(table2);
		
		return false;
succeed:
		;
	}
	
	size_t index = map->hash(key) % map->capacity;
	if(!map->table[index]) {
		map->table[index] = list_create(map->malloc, map->free);
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
	
	MapEntry* entry = map->malloc(sizeof(MapEntry));
	if(!entry)
		return false;
	
	entry->key = key;
	entry->data = data;
	
	if(!list_add(map->table[index], entry)) {
		map->free(entry);
		return false;
	}
	map->size++;
	
	return true;
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
			map->free(entry);
			
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
	MapEntry* entry = list_remove(iter->map->table[iter->index], iter->list_index);
	iter->entry.key = entry->key;
	iter->entry.data = entry->data;
	iter->map->free(entry);
	
	if(list_is_empty(iter->map->table[iter->index])) {
		list_destroy(iter->map->table[iter->index]);
		iter->map->table[iter->index] = NULL;
	/*
	} else {
		iter->list_index++;
	*/
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
