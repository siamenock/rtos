#include <util/cache.h>

static void* get(Cache* this, void* key) {
	HashMap* map = this->map;
	void* data = map->get(map, key);
	if(!data)
		return NULL;

	LinkedList* list = this->list;
	list->remove(list, key);
	list->add(list, key);

	return data;
}

static bool put(Cache* this, void* key, void* value) {
	HashMap* map = this->map;
	if(map->contains_key(map, key))
		return false;

	LinkedList* list = this->list;
	if(list->size >= this->capacity) {
		void* lru_key = list->remove_first(list);
		void* lru_value = map->remove(map, lru_key);
		if(this->type == DATATYPE_POINTER)
			this->free(lru_value);
	}

	list->add(list, key);
	map->put(map, key, value);

	return true;
}

static void* remove(Cache* this, void* key) {
	HashMap* map = this->map;
	void* data = map->remove(map, key);
	if(!data)
		return NULL;

	LinkedList* list = this->list;
	list->remove(list, key);
	if(this->type == DATATYPE_POINTER)
		this->free(data);

	return data;
}

// FIXME: iterate reversely
static void iterator_init(CacheIterContext* context, Cache* cache) {
	cache->list->iter->init(context, cache->list);
}

static bool iterator_has_next(CacheIterContext* context) {
	return context->list->iter->has_next(context);
}

static void* iterator_next(CacheIterContext* context) {
	return context->list->iter->next(context);
}

static void* iterator_remove(CacheIterContext* context) {
	return context->list->iter->remove(context);
}

static Iterator iterator = {
	.init		= (void*)iterator_init,
	.has_next	= (void*)iterator_has_next,
	.next		= (void*)iterator_next,
	.remove		= (void*)iterator_remove,
};

Cache* cache_create(DataType type, PoolType pool, size_t initial_capacity) {
	Cache* cache = (Cache*)map_create(type, pool, sizeof(Cache));
	if(!cache)
		return NULL;

	cache->map = hashmap_create(type, pool, initial_capacity);
	if(!cache->map)
		goto failed;

	cache->list = linkedlist_create(type, pool);
	if(!cache->list)
		goto failed;

	cache->capacity		= initial_capacity;
	cache->iter		= &iterator;

	cache->get		= (void*)get;
	cache->put		= (void*)put;
	cache->remove		= (void*)remove;

	return cache;

failed:
	if(cache->map)
		hashmap_destroy(cache->map);

	map_destroy((Map*)cache);

	return NULL;
}

void cache_destroy(Cache* this) {
	linkedlist_destroy(this->list);
	hashmap_destroy(this->map);
	map_destroy((Map*)this);
}

