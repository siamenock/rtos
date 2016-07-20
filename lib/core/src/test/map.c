#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>

#include <malloc.h>
#include <util/map.h>

#define POOL_SIZE	0x40000
#define MAX_BUCKET_SIZE	0x4000
#define ENTRY_SIZE	100	
#define STRING_SIZE	8
/*  - key_data_set function
 * This function is made for entries to test map function.
 */
static int key[ENTRY_SIZE];
static int data[ENTRY_SIZE];
static char string_key[ENTRY_SIZE][STRING_SIZE];

static void key_data_set() {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		key[i] = i;
		data[i] = i;
		snprintf(string_key[i], STRING_SIZE, "Key %d", i);
	}
}

/*  - hash_check function
 * This function is made for checking index in putting entries.
 */
static uintptr_t hash_check(void* key, size_t initial_capacity) {
	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;

	return (uintptr_t)key % capacity;
}

/*  - test_map_string_hash function
 * This function is made for comparing return value of map_string_has
 * All logic is same with map_string_has
 */
static uint64_t test_map_string_hash(void* key) {
         char* c = key;
         uint32_t len = 0;
         uint32_t sum = 0;
         while(*c != '\0') {
                 len++;
                 sum += *c++;
         }
 
         return ((uint64_t)len) << 32 | (uint64_t)sum;
}

static void map_create_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	for(int i = 1; i < MAX_BUCKET_SIZE; i++) {
		Map* map = map_create(i, NULL, NULL, malloc_pool);
		assert_in_range(map, malloc_pool, malloc_pool + POOL_SIZE);
		map_destroy(map);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_destroy_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	size_t first_size = get_used_size(malloc_pool);
	for(int i = 1; i < MAX_BUCKET_SIZE; i++) {
		Map* map = map_create(i, NULL, NULL, malloc_pool);

		size_t mem_size = get_used_size(malloc_pool);
		assert_int_not_equal(mem_size, first_size);

		map_destroy(map);

		mem_size = get_used_size(malloc_pool);
		assert_int_equal(mem_size, first_size);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_is_empty_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	for(int i = 1; i < MAX_BUCKET_SIZE; i++) {
		Map* map = map_create(i, NULL, NULL, malloc_pool);
		assert_true(map_is_empty(map));

		for(int j = 0; j < ENTRY_SIZE; j++) {
			map_put(map, (void*)&key[j], (void*)&data[j]);
			assert_false(map_is_empty(map));
			map_remove(map, (void*)&key[j]);
		}
		
		assert_true(map_is_empty(map));
		
		map_destroy(map);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_put_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(map_put(map, (void*)&key[i], (void*)&data[i]));
		assert_false(map_put(map, (void*)&key[i], (void*)&data[ENTRY_SIZE - 1 - i]));
	}
	map_destroy(map);
	

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_update_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_false(map_update(map, (void*)&key[i], (void*)&data[i]));
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_true(map_update(map, (void*)&key[i], (void*)&data[99 - i]));
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_get_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_null(map_get(map, (void*)&key[i]));
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_memory_equal(map_get(map, (void*)&key[i]), &data[i], sizeof(void*));
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_get_key_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	// case1: Map's 2nd and 3rd parameter(hash and equals) are NULL
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_null(map_get_key(map, (void*)&key[i]));
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_memory_equal(map_get_key(map, (void*)&key[i]), &key[i], sizeof(void*));
	}
	map_destroy(map);

	// case1: Map's 2nd and 3rd parameter(hash and equals) are string_hash
	map = map_create(MAX_BUCKET_SIZE, map_string_hash, map_string_equals, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		void* temp_key = malloc(sizeof(char) * STRING_SIZE + 1);
		snprintf(temp_key, STRING_SIZE, "Key %d", i);
		
		assert_null(map_get_key(map, temp_key));
		
		map_put(map, (void*)string_key[i], (void*)&data[i]);
		
		assert_memory_equal(map_get_key(map, temp_key), string_key[i], sizeof(void*));
		free(temp_key);
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_contains_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_false(map_contains(map, (void*)&key[i]));
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_true(map_contains(map, (void*)&key[i]));
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_false(map_contains(map, (void*)&key[i]));
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_true(map_contains(map, (void*)&key[i]));
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_capacity_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_null(map_remove(map, (void*)&key[i]));
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_memory_equal(map_remove(map, (void*)&key[i]), &data[i], sizeof(void*));
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_size_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	
	assert_int_equal(map_size(map), 0);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_put(map, (void*)&key[i], (void*)&data[i]);
		assert_int_equal(map_size(map), i + 1);
	}
	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_remove(map, (void*)&key[i]);
		assert_int_equal(map_size(map), 99 - i);
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_iterator_init_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	MapIterator iter;
	
	size_t index[ENTRY_SIZE];
	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_put(map, (void*)&key[i], (void*)&data[i]);
		index[i] = hash_check((void*)&key[i], MAX_BUCKET_SIZE);
	}
		
	map_iterator_init(&iter, map);
	assert_int_equal(iter.list_index, 0);	
		
	for(int i = 0; map_iterator_has_next(&iter); i++) {
		map_iterator_next(&iter);
		
		assert_int_equal(iter.index, index[i]);
	}
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_iterator_has_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	MapIterator iter;
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_put(map, (void*)&key[i], (void*)&data[i]);
	}
		
	map_iterator_init(&iter, map);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(map_iterator_has_next(&iter));
		map_iterator_next(&iter);
	}

	assert_false(map_iterator_has_next(&iter));
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_iterator_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	MapIterator iter;
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_put(map, (void*)&key[i], (void*)&data[i]);
	}
		
	map_iterator_init(&iter, map);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_iterator_has_next(&iter);
		size_t before_list_index = iter.list_index;
		assert_non_null(map_iterator_next(&iter));
		
		assert_int_equal(before_list_index + 1, iter.list_index);
	}
	
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_iterator_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Map* map = map_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	MapIterator iter;
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_put(map, (void*)&key[i], (void*)&data[i]);
	}
		
	map_iterator_init(&iter, map);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		map_iterator_has_next(&iter);
		size_t before_list_index = iter.list_index;
		size_t before_map_size = map->size;
		assert_non_null(map_iterator_next(&iter));
		assert_non_null(map_iterator_remove(&iter));
		
		assert_int_equal(before_list_index, iter.list_index);
		assert_int_equal(before_map_size - 1, map->size);
	}
	
	map_destroy(map);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void map_uint64_hash_func(void **state) {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_int_equal((uintptr_t)&key[i], map_uint64_hash(&key[i]));
	}
}

static void map_uint64_equals_func(void **state) {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(map_uint64_equals(&key[i], &key[i]));
		assert_false(map_uint64_equals(&key[i], &i));
	}
}

static void map_string_hash_func(void **state) {
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		i++;
		assert_int_equal(map_string_hash(&key[i]), test_map_string_hash(&key[i]));
	}	
}

static void map_string_equals_func(void **state) {

	for(int i = 0; i < ENTRY_SIZE; i++) {
		static char temp_string_key[STRING_SIZE];
		snprintf(temp_string_key, STRING_SIZE, "Key %d", i);
		assert_true(map_string_equals(&string_key[i], temp_string_key));
		assert_false(map_string_equals(&string_key[i], "false"));
	}
}
int main(void) {
	key_data_set();
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(map_create_func),
		cmocka_unit_test(map_destroy_func),
		cmocka_unit_test(map_is_empty_func),
		cmocka_unit_test(map_put_func),
		cmocka_unit_test(map_update_func),
		cmocka_unit_test(map_get_func),
		cmocka_unit_test(map_get_key_func),
		cmocka_unit_test(map_contains_func),
		cmocka_unit_test(map_remove_func),
		cmocka_unit_test(map_capacity_func),
		cmocka_unit_test(map_size_func),
		cmocka_unit_test(map_iterator_init_func),
		cmocka_unit_test(map_iterator_has_next_func),
		cmocka_unit_test(map_iterator_next_func),
		cmocka_unit_test(map_iterator_remove_func),
		cmocka_unit_test(map_uint64_hash_func),
		cmocka_unit_test(map_uint64_equals_func),
		cmocka_unit_test(map_string_hash_func),
		cmocka_unit_test(map_string_equals_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
