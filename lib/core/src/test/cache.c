#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <_malloc.h>

#include <stdio.h>
#include <malloc.h>
#include <util/cache.h>

#define POOL_SIZE	0x40000
#define CACHE_CAPACITY	0x4000
#define ENTRY_SIZE	100

static int key[ENTRY_SIZE];
static int data[ENTRY_SIZE];

static void key_data_set() {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		key[i] = i;
		data[i] = i;
	}
}

static void cache_create_func(void **state) {
	(void) state; /* unused */

	Cache* cache;
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	for(size_t i = 1; i < 10000; i++) {
		
		size_t mem_size = i;
		cache = cache_create(mem_size, NULL, malloc_pool);
		assert_in_range(cache, cache, cache + POOL_SIZE);

		cache_destroy(cache);
		cache = NULL;
	}
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_destroy_func(void **state) {
	(void) state; /* unused */

	Cache* cache;
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	size_t first_size = get_used_size(malloc_pool);
	for(size_t i = 1; i < 10000; i++) {
		size_t mem_size = i;
		size_t temp_size;
		cache = cache_create(mem_size, NULL, malloc_pool);
		temp_size = get_used_size(malloc_pool);

		assert_int_not_equal(temp_size, first_size);
		
		cache_destroy(cache);

		temp_size = get_used_size(malloc_pool);
		assert_int_equal(temp_size, first_size);
		cache = NULL;
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_get_func(void **state) {
	(void) state; /* unused */

	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);
	
	// case1: There are no data in cache.
	assert_null(cache_get(cache, (void*)&key[0]));

	// case2: There are data you want in cache.
	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
		assert_memory_equal(cache_get(cache, (void*)&key[i]), (void*)&data[i], sizeof(void*));
	}
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_set_func(void **state) {
	(void) state; /* unused */
	
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(cache_set(cache, (void*)&key[i], (void*)&data[i]));
	}
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_false(cache_set(cache, (void*)&key[i], (void*)&data[i]));
	}
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}


static void cache_remove_func(void **state) {
	(void) state; /* unused */

	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_memory_equal(cache_remove(cache, (void*)&key[i]), (void*)&data[i], sizeof(void*));
		assert_null(cache_get(cache, (void*)&key[i]));
	}
	
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_clear_func(void **state) {
	(void) state; /* unused */
	
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	cache_clear(cache);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_null(cache_get(cache, (void*)&key[i]));
	}
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

}

static void cache_iterator_init_func(void **state) {
	(void) state; /* unused */
	
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	CacheIterator iter;
	cache_iterator_init(&iter, cache);
	
	assert_int_equal(iter.cache, cache);
	assert_int_equal(iter.node, cache->list->head);

	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

	
}

static void cache_iterator_has_next_func(void **state) {
	(void) state; /* unused */
	
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	CacheIterator iter;
	cache_iterator_init(&iter, cache);
	
	assert_false(cache_iterator_has_next(&iter));

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	cache_iterator_init(&iter, cache);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(cache_iterator_has_next(&iter));
	}
	
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}
static void cache_iterator_next_func(void **state) {
	(void) state; /* unused */
	
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	CacheIterator iter;
	cache_iterator_init(&iter, cache);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	cache_iterator_init(&iter, cache);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_int_equal(cache_iterator_next(&iter), (void*)&data[i]);
	}
	for(int i = 707; i < 1000; i++) {
		assert_null(cache_iterator_next(&iter));
	}
	
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}


/*
static void cache_set_constant_buffer(void **state) {
	Cache* cache = cache_create(1000, NULL, NULL);
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(cache_set(cache, (void*)key[i], (void*)&data[i]));
	}
}

static void cache_set_malloc_buffer(void **state) {
	Cache* cache = cache_create(1000, free, NULL);
	for(int i = 0; i < 4001; i++) {
		char* buf = malloc(10000);
		assert_true(cache_set(cache, (void*)(uintptr_t)i, buf));
	}
}*/

int main(void) {
	key_data_set();
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(cache_create_func),
		cmocka_unit_test(cache_destroy_func),
		cmocka_unit_test(cache_get_func),
		cmocka_unit_test(cache_set_func),
		cmocka_unit_test(cache_remove_func),
		cmocka_unit_test(cache_clear_func),
		cmocka_unit_test(cache_iterator_init_func),
		cmocka_unit_test(cache_iterator_has_next_func),
		cmocka_unit_test(cache_iterator_next_func),
//		cmocka_unit_test(cache_set_constant_buffer),
//		cmocka_unit_test(cache_set_malloc_buffer),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
#if 0

int main() {
	void show(int size, Cache* cache) {
		for(int i = 0; i < size; i++) {
			if(!cache_get(cache, (void*)(uintptr_t)i))
				printf("%3d(X)\t", i);
			else
				printf("%3d   \t", i);
		}
		printf("\n");
	}

	Cache* cache;
	int num = 4;

	switch(num) {
		case 0: 
			// Charcater constant free
 			cache = cache_create(1000, NULL, NULL);
			for(int i = 0; i < 1001; i++) {
				if(!cache_set(cache, (void*)(uintptr_t)i, "hello world")) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			break;
		case 1:
			// Malloc free
			cache = cache_create(1000, free, NULL);
			for(int i = 0; i < 4001; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			break;
		case 2:
			// Key update check
			cache = cache_create(1000, free, NULL);
			for(int i = 0; i < 1500; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			for(int i = 0; i < 1000; i++) {
				if(!cache_get(cache, (void*)(uintptr_t)i))
					printf("%d(X)\n", i);
				else
					printf("%d\n", i);
			}

			break;
		case 3: 
			// Cash clear  
			cache = cache_create(200, free, NULL);
			for(int i = 0; i < 200; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			printf("Set after\n");
			for(int i = 0; i < 200; i++) {
				if(!cache_get(cache, (void*)(uintptr_t)i))
					printf("%3d(X)\t", i);
				else
					printf("%3d   \t", i);
			}
			show(200, cache);

			cache_clear(cache);
			printf("Clear after\n");
			show(200, cache);

			for(int i = 0; i < 300; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			printf("Set again after\n");
			show(200, cache);
		case 4: 
			// Destroy & create
			for(int i = 0; i < 1000000; i++) {
				cache = cache_create(200, free, NULL);
				cache_destroy(cache);
			}
			
	}

	return 0;
}
#endif
