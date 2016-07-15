#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <_malloc.h>

#include <malloc.h>
#include <util/list.h>

#define POOL_SIZE	0x40000

/* NOTE: All these test cases uses random value in for-loop,
 *	 because list uses memories dynamically according to adding data.
 */

static void list_create_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	List* list;

	for(int i = 0; i < 5000; i++) {
		list = list_create(malloc_pool);
		assert_in_range(list, malloc_pool, malloc_pool + POOL_SIZE);     
		list_destroy(list);
	}
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_destroy_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	List* list;

	size_t first_size = get_used_size(malloc_pool);
	for(int i = 0; i < 5000; i++) {
		list = list_create(malloc_pool);
		size_t mem_size = get_used_size(malloc_pool);
		assert_int_not_equal(mem_size, first_size);     
		list_destroy(list);

		mem_size = get_used_size(malloc_pool);
		assert_int_equal(mem_size, first_size);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}	

static void list_is_empty_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	List* list;

	list = list_create(malloc_pool);
	assert_true(list_is_empty(list));

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		for(int j = 0; j < 1000; j++) {
			void* data = (void*)&temp_value[j];
			list_add(list, data);
		}
		assert_false(list_is_empty(list));

		for(int j = 0; j < 1000; j++) {
			list_remove_first(list);
		}
		assert_true(list_is_empty(list));
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_add_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);
	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		
		list_add(list, data);
		assert_memory_equal(list_get_last(list), data, sizeof(void*));
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

/* NOTE: list_add_at_func
 *  - For putting in data at random index, this test code uses dummy_index[].
 *  - Every 10 times in for-loop, each dummy_index values increases 3. 
 *  - increasing size(3) is randomly selected size. (but, this size must be under 10)
 */
static void list_add_at_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int dummy_index[10] = { 0, 5, 2, 7, 3, 4, 9, 6, 1, 8 };
	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		int index = dummy_index[i % 10];

		list_add_at(list, index, data);
			
		// In this list, the size of list is same with 'i', because every for-loop list is added 1 entry.
		if(i >= index) 
			assert_memory_equal(list_get(list, index), data, sizeof(void*));
		else
			assert_memory_equal(list_get_last(list), data, sizeof(void*));

		if(i % 10 == 0) {
			for(int j = 0; j < 10; j++) {
				dummy_index[j] += 3;
			}
		}
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

/* NOTE: list_get_func
 *  - Using same logic with list_add_at_func, this test code approach data through random index.
 */
static void list_get_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int dummy_index[10] = { 0, 5, 2, 7, 3, 4, 9, 6, 1, 8 };
	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		int index = dummy_index[i % 10];

		list_add_at(list, index, data);
		if(i >= index)
			assert_memory_equal(list_get(list, index), data, sizeof(void*));
		else
			assert_null(list_get(list, index));

		if(i % 10 == 0) {
			for(int j = 0; j < 10; j++) {
				dummy_index[j] += 3;
			}
		}
	}
	for(int i = 1000; i >= 0; i--) {
		void* data = NULL;
		int index = dummy_index[i % 10];
		
		data = list_get(list, index);
		
		if(i >= index)
			assert_memory_equal(list_remove(list, index), data, sizeof(void*));
		else
			assert_null(list_remove(list, index));
		
		if(i % 10 == 0) {
			for(int j = 0; j < 10; j++) {
				dummy_index[j] -= 3;
			}
		}
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_get_first_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);
	
	assert_null(list_get_first(list));

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[999 - i];
		
		list_add_at(list, 0, data);
		assert_memory_equal(list_get_first(list), data, sizeof(void*));
	}
	
	for(int i = 0; i < 1000; i++) {
		list_remove_first(list);
		
		// There is no entry in list.
		if(i == 999) {
			assert_null(list_get_first(list));
			break;
		}

		assert_memory_equal(list_get_first(list), (void*)&temp_value[i + 1], sizeof(void*));
	}
	
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_get_last_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);
	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		
		list_add(list, data);
		assert_memory_equal(list_get_last(list), data, sizeof(void*));
	}

	for(int i = 0; i < 1000; i++) {
		list_remove_last(list);
		
		// There is no entry in list.
		if(i == 999) {
			assert_null(list_get_last(list));
			break;
		}

		assert_memory_equal(list_get_last(list), (void*)&temp_value[998 - i], sizeof(void*));
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_index_of_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		
		// If there is no data you want in list, return -1
		assert_int_equal(list_index_of(list, data, NULL), -1);
		
		list_add(list, data);
		assert_int_equal(list_index_of(list, data, NULL), i);
	}

	assert_int_equal(list_index_of(list, NULL, NULL), -1);

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		temp_value[i] = i;
	}

	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
	
		list_add(list, data);
	}

	for(int i = 0; i < 1000; i++) {
		assert_memory_equal(list_remove(list, 999 - i), &temp_value[999 - i], sizeof(void*));
		assert_int_equal(list_index_of(list, &temp_value[999 - i], NULL), -1);
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_remove_data_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		temp_value[i] = i;
	}

	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		
		list_add(list, data);
	}

	for(int i = 0; i < 1000; i++) {
		assert_true(list_remove_data(list, &temp_value[i]));
		assert_false(list_remove_data(list, &temp_value[i]));
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_remove_first_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	assert_null(list_remove_first(list));

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		
		list_add(list, data);
	}

	for(int i = 0; i < 1000; i++) {
		assert_memory_equal(list_remove_first(list), &temp_value[i], sizeof(void*));
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_remove_last_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	assert_null(list_remove_last(list));

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		
		list_add(list, data);
	}

	for(int i = 0; i < 1000; i++) {
		assert_memory_equal(list_remove_last(list), &temp_value[999 - i], sizeof(void*));
	}

	assert_null(list_remove_last(list));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_size_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];

		assert_int_equal(list_size(list), i);
		list_add(list, data);
		assert_int_equal(list_size(list), i + 1);
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_rotate_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		list_add(list, data);
	}

	for(int i = 0; i < 1000; i++) {
		assert_memory_equal(list_get_first(list), &temp_value[i], sizeof(void*));
		assert_memory_equal(list_get_last(list), &temp_value[(i + 999) % 1000], sizeof(void*));
		list_rotate(list);
		assert_memory_equal(list_get_first(list), &temp_value[(i + 1) % 1000], sizeof(void*));

		assert_memory_equal(list_get_last(list), &temp_value[i], sizeof(void*));
	}		
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_iterator_init_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		list_add(list, data);
	}

	ListIterator iter;
	list_iterator_init(&iter, list);

	assert_memory_equal(iter.list, list, sizeof(List*));
	assert_null(iter.prev);
	assert_memory_equal(iter.node, list->head, sizeof(ListNode*));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_iterator_has_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	ListIterator iter;
	list_iterator_init(&iter, list);

	assert_false(list_iterator_has_next(&iter));

	void* data = malloc(sizeof(void*));
	list_add(list, data);

	list_iterator_init(&iter, list);
	
	assert_true(list_iterator_has_next(&iter));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_iterator_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);

	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	ListIterator iter;
	list_iterator_init(&iter, list);

	assert_null(list_iterator_next(&iter));

	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		list_add(list, data);
	}

	list_iterator_init(&iter, list);

	for(int i = 0; i < 1000; i++) {
		assert_memory_equal(list_iterator_next(&iter), &temp_value[i], sizeof(void*));
	}

	assert_null(list_iterator_next(&iter));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_iterator_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);

	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	ListIterator iter;
	list_iterator_init(&iter, list);
	assert_null(list_iterator_remove(&iter));
	
	int temp_value[1000];
	for(int i = 0; i < 1000; i++) {
		void* data = (void*)&temp_value[i];
		list_add(list, data);
	}

	list_iterator_init(&iter, list);

	for(int i = 0; i < 1000; i++) {
		void* data = list_iterator_next(&iter);
		assert_memory_equal(list_iterator_remove(&iter), data, sizeof(void*));
	}
	
	assert_false(list_iterator_has_next(&iter));
	
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(list_create_func),
		cmocka_unit_test(list_destroy_func),
		cmocka_unit_test(list_is_empty_func),
		cmocka_unit_test(list_add_func),
		cmocka_unit_test(list_add_at_func),
		cmocka_unit_test(list_get_func),
		cmocka_unit_test(list_get_first_func),
		cmocka_unit_test(list_get_last_func),
		cmocka_unit_test(list_index_of_func),
		cmocka_unit_test(list_remove_func),
		cmocka_unit_test(list_remove_data_func),
		cmocka_unit_test(list_remove_first_func),
		cmocka_unit_test(list_remove_last_func),
		cmocka_unit_test(list_size_func),
		cmocka_unit_test(list_rotate_func),
		cmocka_unit_test(list_iterator_init_func),
		cmocka_unit_test(list_iterator_has_next_func),
		cmocka_unit_test(list_iterator_next_func),
		cmocka_unit_test(list_iterator_remove_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
