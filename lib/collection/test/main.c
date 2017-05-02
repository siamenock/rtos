#include <stdio.h>
#include <util/arraylist.h>
#include <util/linkedlist.h>
#include <util/arrayqueue.h>
#include <util/hashmap.h>
#include <util/hashset.h>
#include <util/cache.h>

int main(int argc, const char *argv[])
{
	ArrayList* list = arraylist_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 1000);
	LinkedList* list2 = linkedlist_create(DATATYPE_UINT64, POOLTYPE_LOCAL);
	ArrayQueue* queue = arrayqueue_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 1000);
	HashMap* map = hashmap_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 16);
	HashSet* set = hashset_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 16);
	Cache* cache = cache_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 16);

	cache->put(cache, (void*)1, (void*)12);
	cache->put(cache, (void*)2, (void*)13);
	cache->put(cache, (void*)3, (void*)14);
	cache->put(cache, (void*)4, (void*)15);
	cache->put(cache, (void*)5, (void*)16);
	cache->put(cache, (void*)6, (void*)17);
	cache->put(cache, (void*)7, (void*)18);
	cache->put(cache, (void*)8, (void*)19);
	cache->put(cache, (void*)9, (void*)20);
	cache->put(cache, (void*)10, (void*)19);
	cache->put(cache, (void*)11, (void*)20);
	cache->put(cache, (void*)12, (void*)19);
	cache->put(cache, (void*)13, (void*)20);
	cache->put(cache, (void*)14, (void*)19);
	cache->put(cache, (void*)15, (void*)20);
	cache->put(cache, (void*)16, (void*)20);
	cache->put(cache, (void*)17, (void*)20);
	cache->put(cache, (void*)18, (void*)20);

	map->put(map, (void*)6, (void*)12);
	map->put(map, (void*)7, (void*)13);
	map->put(map, (void*)8, (void*)14);
	map->put(map, (void*)9, (void*)15);
	map->put(map, (void*)0, (void*)16);
	map->put(map, (void*)4, (void*)17);
	map->put(map, (void*)4, (void*)18);
	map->put(map, (void*)14, (void*)19);
	map->put(map, (void*)24, (void*)20);
	map->put(map, (void*)34, (void*)19);
	map->put(map, (void*)44, (void*)20);
	map->put(map, (void*)54, (void*)19);
	map->put(map, (void*)4, (void*)20);
	map->put(map, (void*)4, (void*)19);
	map->put(map, (void*)4, (void*)20);

	set->add(set, (void*)1);
	set->add(set, (void*)2);
	set->add(set, (void*)3);
	set->add(set, (void*)4);
	set->add(set, (void*)5);
	set->add(set, (void*)6);
	set->add(set, (void*)7);
	set->add(set, (void*)8);
	set->add(set, (void*)12);
	set->add(set, (void*)1);

	Iterator* iter;
	void* element;
	for_each(element, set) {
		printf("%d\n", (int)element);
	}
	for_each(element, map->key_set) {
		printf("%d\n", (int)element);
	}
	for_each(element, cache) {
		printf("%d\n", (int)element);
	}
	return 0;
}
