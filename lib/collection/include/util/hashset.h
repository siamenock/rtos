#ifndef __UTIL_HASH_SET_H__
#define __UTIL_HASH_SET_H__

#include <util/set.h>
#include <util/hashmap.h>

typedef struct _HashSetOps {
	void*	(*get)(void* this, void* key);
} HashSetOps;

typedef struct _HashSetIterContext {
	HashMap*	_map;
	MapIterContext	_context;
} HashSetIterContext;

typedef struct _HashSet {
	Set;
	HashSetOps;

        HashMap*		map;
	HashSetIterContext*	context;
} HashSet;

HashSet* hashset_create(DataType type, PoolType pool, size_t initial_capacity);
void hashset_destroy(HashSet* this);

#endif /* __UTIL_HASH_SET_H__ */
