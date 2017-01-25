#include <string.h>
#include <_malloc.h>
#include <util/shmmap.h>

// TODO: Change accessing list using index to using iterator.

uint64_t shmmap_uint64_hash(void* key);
bool shmmap_uint64_equals(void* key1, void* key2);
uint64_t shmmap_string_hash(void* key);
bool shmmap_string_equals(void* key1, void* key2);

#define THRESHOLD(cap)	(((cap) >> 1) + ((cap) >> 2))	// 75%

Shmmap* shmmap_create(size_t initial_capacity, uint8_t hash_type, uint8_t equals_type, void* pool) {
	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;

	Shmmap* shmmap = __malloc(sizeof(Shmmap), pool);
	if(!shmmap)
		return NULL;

	shmmap->table = __malloc(sizeof(List*) * capacity, pool);
	if(!shmmap->table) {
		__free(shmmap, pool);
		return NULL;
	}

	memset(shmmap->table, 0x0, sizeof(List*) * capacity);
	shmmap->capacity = capacity;
	shmmap->threshold = THRESHOLD(capacity);
	shmmap->size = 0;
	shmmap->hash_type = hash_type;
	shmmap->equals_type = equals_type;
	shmmap->pool = pool;

	return shmmap;
}

static void destroy(Shmmap* shmmap) {
	for(size_t i = 0; i < shmmap->capacity; i++) {
		List* list = shmmap->table[i];
		if(!list)
			continue;
		
		ListIterator iter;
		list_iterator_init(&iter, list);
		while(list_iterator_has_next(&iter)) {
			ShmmapEntry* entry = list_iterator_next(&iter);
			__free(entry, shmmap->pool);
		}
		
		list_destroy(list);
	}
	
	__free(shmmap->table, shmmap->pool);
}

void shmmap_destroy(Shmmap* shmmap) {
	destroy(shmmap);
	__free(shmmap, shmmap->pool);
}

bool shmmap_is_empty(Shmmap* shmmap) {
	return shmmap->size == 0;
}

bool shmmap_put(Shmmap* shmmap, void* key, void* data) {
	if(shmmap->size + 1 > shmmap->threshold) {
		// Create new shmmap
		Shmmap shmmap2;
		size_t capacity = shmmap->capacity * 2;
		shmmap2.table = __malloc(sizeof(List*) * capacity, shmmap->pool);
		if(!shmmap2.table)
			return false;
		memset(shmmap2.table, 0x0, sizeof(List*) * capacity);
		shmmap2.capacity = capacity;
		shmmap2.threshold = THRESHOLD(capacity);
		shmmap2.size = 0;
		shmmap2.hash_type = shmmap->hash_type;
		shmmap2.equals_type = shmmap->equals_type;
		shmmap2.pool = shmmap->pool;
		
		// Copy
		ShmmapIterator iter;
		shmmap_iterator_init(&iter, shmmap);
		while(shmmap_iterator_has_next(&iter)) {
			ShmmapEntry* entry = shmmap_iterator_next(&iter);
			if(!shmmap_put(&shmmap2, entry->key, entry->data)) {
				destroy(&shmmap2);
				return false;
			}
		}
		
		// Destory
		destroy(shmmap);
		
		// Paste
		memcpy(shmmap, &shmmap2, sizeof(Shmmap));
	}
	
	size_t index;
	switch(shmmap->hash_type) {
		case SHMMAP_HASH_TYPE_UINT64:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
		case SHMMAP_HASH_TYPE_STRING:
			index = shmmap_string_hash(key) % shmmap->capacity;
			break;
		default:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
	}

	if(!shmmap->table[index]) {
		shmmap->table[index] = list_create(shmmap->pool);
		if(!shmmap->table[index])
			return false;
	} else {
		size_t size = list_size(shmmap->table[index]);
		for(size_t i = 0; i < size; i++) {
			ShmmapEntry* entry = list_get(shmmap->table[index], i);
			switch(shmmap->equals_type) {
				case SHMMAP_EQUALS_TYPE_UINT64:
					if(shmmap_uint64_equals(entry->key, key))
						return false;
					break;
				case SHMMAP_EQUALS_TYPE_STRING:
					if(shmmap_string_equals(entry->key, key))
						return false;
					break;
				default:
					if(shmmap_uint64_equals(entry->key, key))
						return false;
					break;
			}
		}
	}
	
	ShmmapEntry* entry = __malloc(sizeof(ShmmapEntry), shmmap->pool);
	if(!entry) {
		if(list_is_empty(shmmap->table[index])) {
			list_destroy(shmmap->table[index]);
			shmmap->table[index] = NULL;
		}
		return false;
	}
	
	entry->key = key;
	entry->data = data;
	
	if(!list_add(shmmap->table[index], entry)) {
		__free(entry, shmmap->pool);
		if(list_is_empty(shmmap->table[index])) {
			list_destroy(shmmap->table[index]);
			shmmap->table[index] = NULL;
		}

		return false;
	}
	shmmap->size++;
	
	return true;
}

bool shmmap_update(Shmmap* shmmap, void* key, void* data) {
	size_t index;
	switch(shmmap->hash_type) {
		case SHMMAP_HASH_TYPE_UINT64:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
		case SHMMAP_HASH_TYPE_STRING:
			index = shmmap_string_hash(key) % shmmap->capacity;
			break;
		default:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
	}

	if(!shmmap->table[index]) {
		return false;
	}
	
	size_t size = list_size(shmmap->table[index]);
	for(size_t i = 0; i < size; i++) {
		ShmmapEntry* entry = list_get(shmmap->table[index], i);
		switch(shmmap->equals_type) {
			case SHMMAP_EQUALS_TYPE_UINT64:
				if(shmmap_uint64_equals(entry->key, key)) {
					entry->data = data;
					return true;
				}
				break;
			case SHMMAP_EQUALS_TYPE_STRING:
				if(shmmap_string_equals(entry->key, key)) {
					entry->data = data;
					return true;
				}
				break;
			default:
				if(shmmap_uint64_equals(entry->key, key)) {
					entry->data = data;
					return true;
				}
				break;
		}
	}
	
	return false;
}

void* shmmap_get(Shmmap* shmmap, void* key) {
	size_t index;
	switch(shmmap->hash_type) {
		case SHMMAP_HASH_TYPE_UINT64:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
		case SHMMAP_HASH_TYPE_STRING:
			index = shmmap_string_hash(key) % shmmap->capacity;
			break;
		default:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
	}

	if(!shmmap->table[index]) {
		return NULL;
	}
	
	size_t size = list_size(shmmap->table[index]);
	for(size_t i = 0; i < size; i++) {
		ShmmapEntry* entry = list_get(shmmap->table[index], i);
		switch(shmmap->equals_type) {
			case SHMMAP_EQUALS_TYPE_UINT64:
				if(shmmap_uint64_equals(entry->key, key))
					return entry->data;
				break;
			case SHMMAP_EQUALS_TYPE_STRING:
				if(shmmap_string_equals(entry->key, key))
					return entry->data;
				break;
			default:
				if(shmmap_uint64_equals(entry->key, key))
					return entry->data;
				break;
		}
	}

	return NULL;
}

void* shmmap_get_key(Shmmap* shmmap, void* key) {
	size_t index;
	switch(shmmap->hash_type) {
		case SHMMAP_HASH_TYPE_UINT64:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
		case SHMMAP_HASH_TYPE_STRING:
			index = shmmap_string_hash(key) % shmmap->capacity;
			break;
		default:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
	}

	if(!shmmap->table[index]) {
		return NULL;
	}
	
	size_t size = list_size(shmmap->table[index]);
	for(size_t i = 0; i < size; i++) {
		ShmmapEntry* entry = list_get(shmmap->table[index], i);
		switch(shmmap->equals_type) {
			case SHMMAP_EQUALS_TYPE_UINT64:
				if(shmmap_uint64_equals(entry->key, key))
					return entry->key;
				break;
			case SHMMAP_EQUALS_TYPE_STRING:
				if(shmmap_string_equals(entry->key, key))
					return entry->key;
				break;
			default:
				if(shmmap_uint64_equals(entry->key, key))
					return entry->key;
				break;
		}
	}
	
	return NULL;
}

bool shmmap_contains(Shmmap* shmmap, void* key) {
	size_t index;
	switch(shmmap->hash_type) {
		case SHMMAP_HASH_TYPE_UINT64:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
		case SHMMAP_HASH_TYPE_STRING:
			index = shmmap_string_hash(key) % shmmap->capacity;
			break;
		default:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
	}

	if(!shmmap->table[index]) {
		return false;
	}
	
	size_t size = list_size(shmmap->table[index]);
	for(size_t i = 0; i < size; i++) {
		ShmmapEntry* entry = list_get(shmmap->table[index], i);
		switch(shmmap->equals_type) {
			case SHMMAP_EQUALS_TYPE_UINT64:
				if(shmmap_uint64_equals(entry->key, key))
					return true;
				break;
			case SHMMAP_EQUALS_TYPE_STRING:
				if(shmmap_string_equals(entry->key, key))
					return true;
				break;
			default:
				if(shmmap_uint64_equals(entry->key, key))
					return true;
				break;
		}
	}
	
	return false;
}

void* shmmap_remove(Shmmap* shmmap, void* key) {
	size_t index;
	switch(shmmap->hash_type) {
		case SHMMAP_HASH_TYPE_UINT64:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
		case SHMMAP_HASH_TYPE_STRING:
			index = shmmap_string_hash(key) % shmmap->capacity;
			break;
		default:
			index = shmmap_uint64_hash(key) % shmmap->capacity;
			break;
	}

	if(!shmmap->table[index]) {
		return NULL;
	}
	
	size_t size = list_size(shmmap->table[index]);
	for(size_t i = 0; i < size; i++) {
		ShmmapEntry* entry = list_get(shmmap->table[index], i);
		switch(shmmap->equals_type) {
			case SHMMAP_EQUALS_TYPE_UINT64:
				if(shmmap_uint64_equals(entry->key, key)) {
					void* data = entry->data;
					list_remove(shmmap->table[index], i);
					__free(entry, shmmap->pool);

					if(list_is_empty(shmmap->table[index])) {
						list_destroy(shmmap->table[index]);
						shmmap->table[index] = NULL;
					}

					shmmap->size--;

					return data;
				}
				break;
			case SHMMAP_EQUALS_TYPE_STRING:
				if(shmmap_string_equals(entry->key, key)) {
					void* data = entry->data;
					list_remove(shmmap->table[index], i);
					__free(entry, shmmap->pool);

					if(list_is_empty(shmmap->table[index])) {
						list_destroy(shmmap->table[index]);
						shmmap->table[index] = NULL;
					}

					shmmap->size--;

					return data;
				}
				break;
			default:
				if(shmmap_uint64_equals(entry->key, key)) {
					void* data = entry->data;
					list_remove(shmmap->table[index], i);
					__free(entry, shmmap->pool);

					if(list_is_empty(shmmap->table[index])) {
						list_destroy(shmmap->table[index]);
						shmmap->table[index] = NULL;
					}

					shmmap->size--;

					return data;
				}
				break;
		}
	}
	
	return NULL;
}

size_t shmmap_capacity(Shmmap* shmmap) {
	return shmmap->capacity;
}

size_t shmmap_size(Shmmap* shmmap) {
	return shmmap->size;
}

void shmmap_iterator_init(ShmmapIterator* iter, Shmmap* shmmap) {
	iter->shmmap = shmmap;
	for(iter->index = 0; iter->index < shmmap->capacity && !shmmap->table[iter->index]; iter->index++);
	iter->list_index = 0;
}

bool shmmap_iterator_has_next(ShmmapIterator* iter) {
	if(iter->index >= iter->shmmap->capacity)
		return false;
	
	if(iter->shmmap->table[iter->index] && iter->list_index < list_size(iter->shmmap->table[iter->index]))
		return true;
	
	for(iter->index++; iter->index < iter->shmmap->capacity && !iter->shmmap->table[iter->index]; iter->index++);
	
	if(iter->index < iter->shmmap->capacity) {
		iter->list_index = 0;
		return true;
	} else {
		return false;
	}
}

ShmmapEntry* shmmap_iterator_next(ShmmapIterator* iter) {
	ShmmapEntry* entry = list_get(iter->shmmap->table[iter->index], iter->list_index++);
	iter->entry.key = entry->key;
	iter->entry.data = entry->data;
	
	return &iter->entry;
}

ShmmapEntry* shmmap_iterator_remove(ShmmapIterator* iter) {
	iter->list_index--;
	ShmmapEntry* entry = list_remove(iter->shmmap->table[iter->index], iter->list_index);
	iter->entry.key = entry->key;
	iter->entry.data = entry->data;
	__free(entry, iter->shmmap->pool);
	
	if(list_is_empty(iter->shmmap->table[iter->index])) {
		list_destroy(iter->shmmap->table[iter->index]);
		iter->shmmap->table[iter->index] = NULL;
	}
	
	iter->shmmap->size--;
	
	return &iter->entry;
}

uint64_t shmmap_uint64_hash(void* key) {
	return (uintptr_t)key;
}

bool shmmap_uint64_equals(void* key1, void* key2) {
	return key1 == key2;
}

uint64_t shmmap_string_hash(void* key) {
	char* c = key;
	uint32_t len = 0;
	uint32_t sum = 0;
	while(*c != '\0') {
		len++;
		sum += *c++;
	}
	
	return ((uint64_t)len) << 32 | (uint64_t)sum;
}

bool shmmap_string_equals(void* key1, void* key2) {
	char* c1 = key1;
	char* c2 = key2;
	
	while(*c1 != '\0' && *c2 != '\0') {
		if(*c1++ != *c2++)
			return false;
	}

	if(*c1 != '\0' || *c2 != '\0')
		return false;
	
	return true;
}
