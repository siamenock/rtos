#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>

#include <_malloc.h>
#include <lock.h>
#include <util/fifo.h>
#include <net/nic.h>
#include <net/packet.h>

#include <stdlib.h>
#include <pthread.h>

#define POOL_SIZE	0x40000
#define THREAD_NUM	40
#define BUFFER_NUM  10

extern int __nic_count;
extern NIC* __nics[NIC_SIZE];


static pthread_t thds[THREAD_NUM];

static void nic_count_func(void** state) {
	__nic_count = NIC_SIZE;

	for(int i = 0; i < NIC_SIZE; i++) {
		int comp_nic_count = __nic_count;
		assert_int_equal(nic_count(), comp_nic_count);
		__nic_count--;
	}
}

static void nic_get_func(void** state) {

	__nic_count = 0;
	for(int i = 0; i < NIC_SIZE; i++) {
		assert_null(nic_get(i));

		__nics[i] = malloc(sizeof(NIC));
		__nic_count++;

		assert_memory_equal(nic_get(i), __nics[i], sizeof(NIC));
	}

	for(int i = 0; i < NIC_SIZE; i++)
		free(__nics[i]);
}

static void nic_alloc_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	for(int i = 0; i < 1; i++) {
		__nics[i] = malloc(sizeof(NIC));
		__nic_count++;

		__nics[i]->pool_size = POOL_SIZE;
		__nics[i]->pool = malloc_pool;

		for(int j = 0; j < 100; j++) {
			Packet* packet = nic_alloc(__nics[i], j);
			assert_in_range(packet, malloc_pool, malloc_pool + POOL_SIZE);
			nic_free(packet);
		}
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_free_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	for(int i = 0; i < 1; i++) {
		__nics[i] = malloc(sizeof(NIC));
		__nic_count++;

		__nics[i]->pool_size = POOL_SIZE;
		__nics[i]->pool = malloc_pool;

		size_t first_size = get_used_size(malloc_pool);
		for(int j = 0; j < 100; j++) {
			Packet* packet = nic_alloc(__nics[i], j);
			size_t mem_size = get_used_size(malloc_pool);
			// Checking memory pool size at first and after creating map.

			assert_int_not_equal(mem_size, first_size);

			nic_free(packet);

			mem_size = get_used_size(malloc_pool);
			// Checking first size of memory pool is same with present get used size after free.
			assert_int_equal(mem_size, first_size);
		}
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_has_input_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->input_buffer = fifo_create(10000, malloc_pool);

	assert_false(nic_has_input(__nics[0]));

	for(int i = 0; i < 100; i++) {
		Packet* packet = nic_alloc(__nics[0], i);
		fifo_push(__nics[0]->input_buffer, packet);
	}

	assert_true(nic_has_input(__nics[0]));

	for(int i = 0; i < 100; i++) {
		Packet* packet = fifo_pop(__nics[0]->input_buffer);
		nic_free(packet);
	}

	assert_false(nic_has_input(__nics[0]));

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static int read_flag;
static uint8_t volatile read_lock = 0;
static volatile int readcount = 0;

static void* thread_nic_input(void* arg) {
	for(int i = 0; i < BUFFER_NUM; i++) {
		if(readcount == 0) {
			lock_lock(&(__nics[0]->input_lock));

			Packet* packet = nic_alloc(__nics[0], 64);
			fifo_push(__nics[0]->input_buffer, packet);
			read_flag++;

			lock_unlock(&(__nics[0]->input_lock));
		}
	}
	return NULL;
}

static void nic_input_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->input_buffer = fifo_create(20000, malloc_pool);
	lock_init(&(__nics[0]->input_lock));

	read_flag = 0;
	for(int i = 0; i < THREAD_NUM; i++) {
		pthread_create(&thds[i], NULL, &thread_nic_input, NULL);
	}

	static int checknum = BUFFER_NUM * THREAD_NUM;
	for(int i = 0; i < checknum; i++) {
		if(nic_has_input(__nics[0])) {
			readcount++;
			while(readcount != 0) {
				Packet* packet = nic_input(__nics[0]);
				assert_true(packet);
				assert_int_equal(sizeof(*packet), sizeof(Packet));
				nic_free(packet);
				read_flag--;

				if(read_flag == 0) {
					readcount--;
				}
			}
		}
	}

	for(int i = 0; i < THREAD_NUM; i++) {
		pthread_join(thds[i], NULL);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_tryinput_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->input_buffer = fifo_create(20000, malloc_pool);
	lock_init(&(__nics[0]->input_lock));

	read_flag = 0;
	for(int i = 0; i < THREAD_NUM; i++) {
		pthread_create(&thds[i], NULL, &thread_nic_input, NULL);
	}
	lock_lock(&(__nics[0]->input_lock));
	assert_null(nic_tryinput(__nics[0]));

	lock_unlock(&(__nics[0]->input_lock));

	for(int i = 0; i < THREAD_NUM; i++) {
		pthread_join(thds[i], NULL);
	}

	for(int i = 0; i < read_flag; i++) {
		Packet* packet = nic_tryinput(__nics[0]);
		assert_true(packet);
		assert_int_equal(sizeof(*packet), sizeof(Packet));
		nic_free(packet);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_output_available_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->output_buffer = fifo_create(2, malloc_pool);
	lock_init(&(__nics[0]->output_lock));

	Packet* packet = nic_alloc(__nics[0], 100);
	assert_true(nic_output_available(__nics[0]));

	nic_output(__nics[0], packet);
	assert_false(nic_output_available(__nics[0]));

	nic_free(packet);
	fifo_destroy(__nics[0]->output_buffer);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_output_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->output_buffer = fifo_create(20000, malloc_pool);
	lock_init(&(__nics[0]->output_lock));

	for(int i = 0; i < 100; i++) {
		Packet* packet = nic_alloc(__nics[0], i);
		assert_int_equal(i, fifo_size(__nics[0]->output_buffer));

		nic_output(__nics[0], packet);
		assert_int_equal(i + 1, fifo_size(__nics[0]->output_buffer));

		nic_free(packet);
	}
	fifo_destroy(__nics[0]->output_buffer);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_output_dup_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->output_buffer = fifo_create(2, malloc_pool);
	lock_init(&(__nics[0]->output_lock));

	Packet* packet = nic_alloc(__nics[0], 100);
	assert_true(nic_output_available(__nics[0]));

	nic_output_dup(__nics[0], packet);
	assert_false(nic_output_available(__nics[0]));

	nic_free(packet);
	fifo_destroy(__nics[0]->output_buffer);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_tryoutput_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->output_buffer = fifo_create(2, malloc_pool);
	lock_init(&(__nics[0]->output_lock));
	lock_lock(&(__nics[0]->output_lock));

	assert_false(nic_tryoutput(__nics[0], NULL));

	lock_unlock(&(__nics[0]->output_lock));

	Packet* packet = nic_alloc(__nics[0], 100);
	assert_true(nic_tryoutput(__nics[0], packet));
	nic_free(packet);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_pool_used_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;


	size_t first_size = nic_pool_used(__nics[0]);

//	__nics[0]->output_buffer = fifo_create(2, malloc_pool);

	Packet* packet = nic_alloc(__nics[0], 100);

	size_t after_size = nic_pool_used(__nics[0]);
	assert_int_not_equal(after_size - first_size, 0);
	
	nic_free(packet);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_pool_free_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;


	size_t first_size = nic_pool_free(__nics[0]);

	__nics[0]->output_buffer = fifo_create(2, malloc_pool);

	Packet* packet = nic_alloc(__nics[0], 100);

	size_t after_size = nic_pool_free(__nics[0]);
	assert_int_not_equal(first_size - after_size, 0);
	
	nic_free(packet);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_pool_total_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;


	size_t first_size = nic_pool_total(__nics[0]);
	
	assert_int_equal(first_size, POOL_SIZE);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_config_put_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	char key[100][8];
	int data[100];

	for(int i = 0; i < 100; i++) {
		sprintf(key[i], "key %d\0", i);
		assert_true(nic_config_put(__nics[0], key[i], &data[i]));
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_config_contains_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	char key[100][8];
	int data[100];

	for(int i = 0; i < 100; i++) {
		sprintf(key[i], "key %d\0", i);
		assert_false(nic_config_contains(__nics[0], key[i]));
		nic_config_put(__nics[0], key[i], &data[i]);
		assert_true(nic_config_contains(__nics[0], key[i]));
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_config_remove_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	char key[100][8];
	int data[100];

	for(int i = 0; i < 100; i++) {
		sprintf(key[i], "key %d\0", i);
		assert_null(nic_config_remove(__nics[0], key[i]));
		nic_config_put(__nics[0], key[i], &data[i]);
		assert_memory_equal(nic_config_remove(__nics[0], key[i]), &data[i], sizeof(int));
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_config_get_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	char key[100][8];
	int data[100];

	for(int i = 0; i < 100; i++) {
		sprintf(key[i], "key %d\0", i);
		assert_null(nic_config_get(__nics[0], key[i]));
		nic_config_put(__nics[0], key[i], &data[i]);
		assert_memory_equal(nic_config_get(__nics[0], key[i]), &data[i], sizeof(int));
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_ip_add_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	uint32_t addr = 127;
	assert_true(nic_ip_add(__nics[0], addr));

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_ip_get_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	uint32_t addr[100];
	for(int i = 0; i < 100; i++) {
		addr[i] = i;
		nic_ip_add(__nics[0], addr[i]);
		IPv4Interface* comp_interface = nic_ip_get(__nics[0], addr[i]);		
		int comp_gateway = (addr[i] & 0xffffff00) | 0x1;
		assert_int_equal(comp_interface->gateway, comp_gateway);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void nic_ip_remove_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);
	
	uint32_t addr[100];
	for(int i = 0; i < 100; i++) {
		addr[i] = i;
		assert_false(nic_ip_remove(__nics[0], addr[i]));
		nic_ip_add(__nics[0], addr[i]);
		assert_true(nic_ip_remove(__nics[0], addr[i]));
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}


int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(nic_count_func),
		cmocka_unit_test(nic_get_func),
		cmocka_unit_test(nic_alloc_func),
		cmocka_unit_test(nic_free_func),
		cmocka_unit_test(nic_has_input_func),
		cmocka_unit_test(nic_input_func),
		cmocka_unit_test(nic_tryinput_func),
		cmocka_unit_test(nic_output_available_func),
		cmocka_unit_test(nic_output_func),
		cmocka_unit_test(nic_output_dup_func),
		cmocka_unit_test(nic_tryoutput_func),
		cmocka_unit_test(nic_pool_used_func),
		cmocka_unit_test(nic_pool_free_func),
		cmocka_unit_test(nic_pool_total_func),
		cmocka_unit_test(nic_config_put_func),
		cmocka_unit_test(nic_config_contains_func),
		cmocka_unit_test(nic_config_remove_func),
		cmocka_unit_test(nic_config_get_func),
		cmocka_unit_test(nic_ip_add_func),
		cmocka_unit_test(nic_ip_get_func),
		cmocka_unit_test(nic_ip_remove_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
