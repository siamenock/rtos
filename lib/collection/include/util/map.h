#ifndef __UTIL_MAP_H__
#define __UTIL_MAP_H__

#include <util/base.h>
#include <util/set.h>

typedef struct _MapEntry {
	void*	key;
	void*	value;
} MapEntry;

typedef struct _Map Map;

typedef struct _MapOps {
	bool	(*is_empty)(void* this);
	void*	(*get)(void* this, void* key);
	bool	(*put)(void* this, void* key, void* value);
	bool	(*update)(void* this, void* key, void* value);
	void*	(*remove)(void* this, void* key);
	bool	(*contains_key)(void* this, void* key);
	bool	(*contains_value)(void* this, void* value);
} MapOps;

typedef struct _MapIterContext {
	void*		_map;
	size_t		index;
	size_t		list_index;
	MapEntry	entry;
} MapIterContext;

typedef struct _EntrySet {
	Set;
	void*		map;
	MapIterContext*	context;
} EntrySet;

typedef struct _EntrySet KeySet;
typedef struct _EntrySet ValueSet;

typedef struct _Map {
	Base;
	MapOps;

	EntrySet*	entry_set;
	KeySet*		key_set;
	ValueSet*	value_set;
	size_t		size;
} Map;

Map* map_create(DataType type, PoolType pool, size_t size);
void map_destroy(Map* this);

#endif /* __UTIL_MAP_H__ */
