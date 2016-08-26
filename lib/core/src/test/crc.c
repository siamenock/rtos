#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/crc.h>

/**
 * TODO: Mock is needed for checking that rtn value is right return value of crc32 func.
 *		comp_rtn takes return value of mock function.
 */

static void crc32_func(void** state) {
	uint8_t data = 0;	//random value
	uint32_t length = 1;
	uint32_t rtn = crc32(&data, length);
	uint32_t comp_rtn = crc32(&data, length);
	assert_int_equal(rtn, comp_rtn);

}

static void crc32_update_func(void** state) {
	uint8_t data = 0;
	uint32_t length = 1;
	uint32_t rtn = crc32_update(0xffffffff, &data, length);
	uint32_t comp_rtn = crc32_update(0xffffffff, &data, length);
	assert_int_equal(rtn, comp_rtn);

}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(crc32_func),
		cmocka_unit_test(crc32_update_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
