#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/checksum.h>

/** 
 * TODO: Conerned with comp_rtn, mock fun is needed for checking rtn value is right or not.
 */
static void checksum_func(void** state) {
	uint32_t data = 0;
	uint32_t comp_data = 0;

	uint32_t rtn = checksum(&data, sizeof(data));
	uint32_t comp_rtn = checksum(&comp_data, sizeof(comp_data));

	assert_int_equal(rtn ,comp_rtn);
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(checksum_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
