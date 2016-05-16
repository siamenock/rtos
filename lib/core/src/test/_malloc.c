#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <_malloc.h>

#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>

extern void* __malloc_pool;

/*
static void malloc_const_size_null(void** state) {
	size_t pool_size = 0x40000;
	__malloc_pool = malloc(pool_size);
	init_memory_pool(pool_size, __malloc_pool, 0);
	
	size_t mem_size = 0;
	void* mem = NULL;

	mem = __malloc(mem_size, NULL);
	assert_null(mem);
	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

static void malloc_const_over_pool() {
	size_t pool_size = 0x40000;
	__malloc_pool = malloc(pool_size);
	init_memory_pool(pool_size, __malloc_pool, 0);

	for(int i = 1; i < 1000; i++) {
		size_t mem_size  = 0x40000 + i;
		void* mem = __malloc(mem_size, NULL);

		assert_null(mem);
	}
	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

static void malloc_const_over_chunk() {
	size_t pool_size = 0x40000;
	for(int i = 1; i < pool_size; i++) {
		__malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __malloc_pool, 0);

		void* used = __malloc(i, NULL);
		void* mem = __malloc(pool_size - i + 1, NULL);

		assert_null(mem);

		free(__malloc_pool);
		__malloc_pool = NULL;
	}
}

static void malloc_stress_below_pool() {
	size_t pool_size = 0x40000;
	size_t malloc_size = 0x11;

	for(int i = 0; i < 4000; i++){
		__malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __malloc_pool, 0);

		uint64_t sum = 0;

		while(1) {
			sum += malloc_size;
			if(sum > pool_size) {
				sum -= malloc_size;
				break;
			} else {
				void* use = __malloc(malloc_size, NULL);	
				assert_in_range(use, __malloc_pool, __malloc_pool + pool_size);
			}
		}
		
		void* last = __malloc(pool_size - sum, NULL);
		assert_in_range(last, __malloc_pool, __malloc_pool + pool_size);

		free(__malloc_pool);
		__malloc_pool = NULL;	
	}
}

static void malloc_stress_all_pool() {

}
*/

static void malloc_func(void** state) {
	size_t pool_size = 0x40000;
/*
	void* malloc_pool = malloc(pool_size);
	init_memory_pool(pool_size, malloc_pool, 0);
	
	size_t mem_size = 0x3e000;
	void* mem = __malloc(mem_size, malloc_pool);
	assert_in_range(mem, malloc_pool, malloc_pool + pool_size);	

	printf("extra malloc\n");
	void* mem2 = __malloc(1800, malloc_pool);
	assert_in_range(mem2, malloc_pool, malloc_pool + pool_size);	

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

	return;
*/

	/*
	 * 0x3e000 is max size that won't make failure of malloc
	 * when pool size is set to 0x4000
	 * If malloc size over the 0x3e000(may be 0x3e001), malloc failed.
	 * more problems is descirbed in Packetngin App Test Sheet.
	 */

	/* use private malloc_pool */
	for(size_t i = 1; i < pool_size - 0x3e001; i++) {
		void* malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, malloc_pool, 0);
		
		size_t mem_size = i;
		void* mem = __malloc(mem_size, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + pool_size);	

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}	

	/* use __malloc_pool */
	for(size_t i = 1; i < pool_size - 0x3e001; i++) {
		__malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __malloc_pool, 0);
		
		size_t mem_size = i;
		void* mem = __malloc(mem_size, NULL);

		assert_in_range(mem, __malloc_pool, __malloc_pool + pool_size);	

		destroy_memory_pool(__malloc_pool);
		free(__malloc_pool);
		__malloc_pool = NULL;
	}	
}

static void free_func() {
	size_t pool_size = 0x40000;
	size_t malloc_size = 253951;
	void* mem;

	/* use private pool */
	void* malloc_pool = malloc(pool_size);
	init_memory_pool(pool_size, malloc_pool, 0);
	
	mem = __malloc(malloc_size, malloc_pool);			
	__free(mem, malloc_pool);
	mem = NULL;

	mem = __malloc(malloc_size, malloc_pool);
	assert_non_null(mem);
	__free(mem, malloc_pool);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

	/* use __malloc_pool */
	__malloc_pool = malloc(pool_size);
	init_memory_pool(pool_size, __malloc_pool, 0);	

	mem = __malloc(malloc_size, __malloc_pool);
	__free(mem, __malloc_pool);
	mem = NULL;

	mem = __malloc(malloc_size, __malloc_pool);
	assert_non_null(mem);
	__free(mem, __malloc_pool);
	mem = NULL;

	destroy_memory_pool(__malloc_pool);
	free(__malloc_pool);
	__malloc_pool = NULL;
}

static void realloc_func() {
	size_t pool_size = 0x40000;
	size_t malloc_size;

	for(size_t i = 1; i < pool_size - 0x3e001; i++) {
		malloc_size = i;

		void* malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, malloc_pool, 0);

		void* mem = __malloc(malloc_size, malloc_pool);		
		assert_in_range(mem, malloc_pool, malloc_pool + pool_size);

		mem = __realloc(mem, 0x3e000, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + pool_size);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}
}

static void calloc_func() {
	size_t pool_size = 0x40000;	
	size_t malloc_size;

	/* element size 1 */	
	for(size_t i = 1; i < pool_size - 0x3e001; i++) {
		malloc_size = i;
		
		void* malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, malloc_pool, 0);

		char* mem = __calloc(malloc_size, 1, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + pool_size);
	
		for(size_t j = 0; j < malloc_size; j++) 
			assert_int_equal(0, (int)mem[j]);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}

	/* element size 4 */
	for(size_t i = 1; i < (pool_size - 0x3e001) / 4; i++) {
		malloc_size = i;
		
		void* malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, malloc_pool, 0);

		int* mem = __calloc(malloc_size, 4, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + pool_size);
	
		for(size_t j = 0; j < malloc_size; j++) 
			assert_int_equal(0, (int)mem[j]);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}


	/* element size 8 */
	for(size_t i = 1; i < (pool_size - 0x3e001) / 8; i++) {
		malloc_size = i;
		
		void* malloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, malloc_pool, 0);

		uint64_t* mem = __calloc(malloc_size, 8, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + pool_size);
	
		for(size_t j = 0; j < malloc_size; j++) 
			assert_int_equal(0, mem[j]);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}

}

int main() {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(malloc_func),
		cmocka_unit_test(free_func),
		cmocka_unit_test(realloc_func),
		cmocka_unit_test(calloc_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
