#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/md5.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ENTRY_CNT		6	//random value
#define MSG_SIZE		64	//random value

/**
 * NOTE: Mock is needed for checking checking that the value of hash array members is right value of md5 func.
 *		comp_hash takes side effect of md5_compress function.
 */
static void md5_func(void** state) {
	uint8_t* message = (uint8_t*)malloc(MSG_SIZE);
	uint8_t* comp_mes = (uint8_t*)malloc(MSG_SIZE);

	for(int i = 0; i < ENTRY_CNT; i++) {
		snprintf((char*)message, MSG_SIZE, "Message %d", i);
		snprintf((char*)comp_mes, MSG_SIZE, "Message %d", i);
	}
	for(int i = 0; i < ENTRY_CNT; i++) {
		uint32_t hash[4];
		uint32_t comp_hash[4];

		md5(message, strlen((char*)message), hash);
		md5(comp_mes, strlen((char*)comp_mes), comp_hash);

		for(int i = 0; i < 4; i++) {
			assert_int_equal(hash[i], comp_hash[i]);
		}
	}
	free(message);
	free(comp_mes);
}

static void md5_blocks_func(void** state) {
	void** blocks = (void**)malloc(ENTRY_CNT * sizeof(void*));
	void** comp_blocks = (void**)malloc(ENTRY_CNT * sizeof(void*));
	for(int i = 0; i < ENTRY_CNT; i++) {
		blocks[i] = malloc(MSG_SIZE);
		comp_blocks[i] = malloc(MSG_SIZE);

		memset(blocks[i], 0, MSG_SIZE);
		memset(comp_blocks[i], 0, MSG_SIZE);

		snprintf((char*)blocks[i], MSG_SIZE, "message_Blocks %03d", i);
		snprintf((char*)comp_blocks[i], MSG_SIZE, "message_Blocks %03d", i);
	}
	uint32_t block_count = ENTRY_CNT;
	uint32_t block_size = MSG_SIZE; 
	uint64_t len = ENTRY_CNT * MSG_SIZE;
	uint32_t hash[4];
	uint32_t comp_hash[4];

	md5_blocks(comp_blocks, block_count, block_size, len, comp_hash);
	md5_blocks(blocks, block_count, block_size, len, hash);
	for(int i = 0; i < 4; i++) {
		assert_int_equal(hash[i], comp_hash[i]);
	}
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(md5_func),
		cmocka_unit_test(md5_blocks_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
