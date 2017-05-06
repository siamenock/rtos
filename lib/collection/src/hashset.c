#include <util/hashset.h>

static char dummy_element;

static bool contains(HashSet* this, void* element) {
	return this->map->contains_key(this->map, element);
}

static bool add(HashSet* this, void* element) {
	return this->map->put(this->map, element, &dummy_element);
}

static bool remove(HashSet* this, void* element) {
	return this->map->remove(this->map, element) == &dummy_element ? true: false;
}

static void* get(HashSet* this, void* key) {
	return this->map->get(this->map, key);
}

static void iterator_init(HashSetIterContext* context, HashSet* set) {
	HashMap* map = context->_map = set->map;
	KeySet* key_set = map->key_set;
	key_set->iter->init(&context->_context, key_set);
}

static bool iterator_has_next(HashSetIterContext* context) {
	HashMap* map = context->_map;
	return map->key_set->iter->has_next(&context->_context);
}

static void* iterator_next(HashSetIterContext* context) {
	HashMap* map = context->_map;
	return map->key_set->iter->next(&context->_context);
}

static void* iterator_remove(HashSetIterContext* context) {
	HashMap* map = context->_map;
	return map->key_set->iter->remove(&context->_context);
}

static Iterator iterator = {
	.init		= (void*)iterator_init,
	.has_next	= (void*)iterator_has_next,
	.next		= (void*)iterator_next,
	.remove		= (void*)iterator_remove,
};

HashSet* hashset_create(DataType type, PoolType pool, size_t initial_capacity) {
	HashSet* set = (HashSet*)set_create(type, pool, sizeof(HashSet));
	if(!set)
		return NULL;

	set->capacity	= initial_capacity;
	set->map	= hashmap_create(type, pool, initial_capacity);
	if(!set->map) {
		hashset_destroy(set);
		return NULL;
	}

	set->iter	= &iterator;
	set->contains	= (void*)contains;
	set->add	= (void*)add;
	set->remove	= (void*)remove;
	set->get	= (void*)get;

	return set;
}

void hashset_destroy(HashSet* this) {
	hashmap_destroy(this->map);
	set_destroy((Set*)this);
}
