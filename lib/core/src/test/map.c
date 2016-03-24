#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <malloc.h>
#include <util/list.h>

static void map_test(void **state) {
	assert_true(true);
}

int main(void) {
	cmocka_set_message_output(CM_OUTPUT_XML);

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(map_test),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
