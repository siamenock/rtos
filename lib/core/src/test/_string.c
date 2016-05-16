#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>

#include <_string.h>

static void memset_func() {
	char* val;
	for(int i = 1; i < 4000; i++) {
		val = malloc(i * sizeof(char));
		for(int j = -128; j < 128; j++) {
			__memset(val, j, i * sizeof(char));
			for(int x = 0; x < i; x++) {
				assert_int_equal(j, val[x]);
			}
		}

		free(val);
		val = NULL;
	}

	/*
	int* int_val;
	for(int i = 1; i < 4000; i++) {
		int_val = malloc(i * sizeof(int));
		for(int j = INT32_MIN; j < INT32_MAX; j++) {
			__memset(int_val, j, i * sizeof(int));
			for(int x = 0; x < i; x++) {
				printf("(i: %d, j: %d) val[%d]: %d\n", i, j, x, int_val[x]);
				assert_int_equal(j, int_val[x]);
			}
		}

		free(int_val);
		int_val = NULL;
	}

	long* long_val;
	for(int i = 1; i < 4000; i++) {
		long_val = malloc(i * sizeof(long));
		for(long j = LONG_MIN; j < LONG_MAX; j++) {
			__memset(val, j, i);
			for(int x = 0; x < i; x++) {
				assert_int_equal(j, val[x]);
			}
		}

		free(long_val);
		long_val = NULL;
	}
	*/
}

static void memset_sse_func() {
	char* val;
	for(int i = 1; i < 4000; i++) {
		val = malloc(i * sizeof(char));
		for(int j = -128; j < 128; j++) {
			__memset_sse(val, j, i * sizeof(char));
			for(int x = 0; x < i; x++) {
				printf("i: %d / j: %d / val[%d]: %d\n", i, j, x, val[x]);
				assert_int_equal(j, val[x]);
			}
		}

		free(val);
		val = NULL;
	}
}

static void memcpy_func() {
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);
	
		assert_not_in_range(dst, src, src + i);
		assert_not_in_range(src, dst, dst + i);

		for(int j = 0; j < i; j++) {
			__memcpy(dst, src, j);		
			
			for(int x = 0; x < j; x++) {
				assert_int_equal(dst[x], src[x]);
			}
		}	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}	
}

static void memcpy_sse_func() {
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);
	
		assert_not_in_range(dst, src, src + i);
		assert_not_in_range(src, dst, dst + i);

		for(int j = 0; j < i; j++) {
			__memcpy(dst, src, j);		
			
			for(int x = 0; x < j; x++) {
				assert_int_equal(dst[x], src[x]);
			}
		}	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}	
}

static void memmove_func() {
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);
	
		assert_not_in_range(dst, src, src + i);
		assert_not_in_range(src, dst, dst + i);

		for(int j = 0; j < i; j++) {
			__memmove(dst, src, j);		
			
			for(int x = 0; x < j; x++) {
				assert_int_equal(dst[x], src[x]);
			}
		}	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}	
}

static void memmove_sse_func() {
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);
	
		assert_not_in_range(dst, src, src + i);
		assert_not_in_range(src, dst, dst + i);

		for(int j = 0; j < i; j++) {
			__memmove_sse(dst, src, j);		
			
			for(int x = 0; x < j; x++) {
				assert_int_equal(dst[x], src[x]);
			}
		}	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}	
}

static void memcmp_func() {
	char* src;
	char* dst;

	for(int i = 0; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);

		memset(src, 0, i);		
		memset(dst, 0, i);		
		
		int rtn = __memcmp(src, dst, i);
		assert_int_equal(0, rtn);	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}
}

static void memcmp_sse_func() {
	char* src;
	char* dst;

	for(int i = 0; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);

		memset(src, 0, i);		
		memset(dst, 0, i);		
		
		int rtn = __memcmp_sse(src, dst, i);
		assert_int_equal(0, rtn);	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}
}

static void bzero_func() {
	char* val;
	for(int i = 0; i < 4000; i++) {
		val = malloc(i);
		__bzero(val, i);						

		for(int j = 0; j < i; j++) {
			assert_int_equal(0, val[j]);
		}
	}
}	

static void strlen_func() {
	char* str;
	for(int i = 1; i < 4000; i++) {
		str = malloc(i + 1);
		memset(str, 1, i);	
		str[i] = '\0';	
		
		int len = __strlen(str);
		assert_int_equal(i, len);
	}
}

static void strstr_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";

	/* head */
	char* str = __strstr(text, "`1234567890");		
	assert_memory_equal(str, "`1234567890", strlen("`1234567890"));	
	
	/* middle */
	str = __strstr(text, "-=qwertyuiop[]asdfghjk");
	assert_memory_equal(str, "-=qwertyuiop[]asdfghjk", strlen("-=qwertyuiop[]asdfghjk"));	

	/* tail */
	str = __strstr(text, "-=qwertyuiop[]asdfghjk");
	assert_memory_equal(str, "-=qwertyuiop[]asdfghjk", strlen("-=qwertyuiop[]asdfghjk"));	

	/* all */
	str = __strstr(text, "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0");
	assert_memory_equal(str, text, strlen(text));

	/* not matched */
	str = __strstr(text, "11`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_\0");
	assert_null(str);
}

static void strchr_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	int len = strlen(text);

	for(int i = 0; i < len; i++) {
		char* str = __strchr(text, text[i]);				
		assert_memory_equal(str, text + i, strlen(text + i));		
	}
}

static void strrchr_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:1ZXCVBNM<>?\0";
	int len = strlen(text);

	for(int i = 0; i < len; i++) {
		char* str;
		if(text[i] == '1') {
			str = __strrchr(text, '1');		
			assert_memory_equal(str, "1ZXCVBNM<>?", strlen("1ZXCVBNM<>?"));
		} else {
			str = __strrchr(text, text[i]);				
			assert_memory_equal(str, text + i, strlen(text + i));		
		}
	}	
}

static void strcmp_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char equal[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char not_eqaul_1[1024] = "11234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char not_eqaul_2[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,.!~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char not_eqaul_3[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,.!~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNMM?!\0";

	assert_int_equal(0, __strcmp(text, equal));	

	assert_int_not_equal(0, __strcmp(text, not_eqaul_1));	
	assert_int_not_equal(0, __strcmp(text, not_eqaul_2));	
	assert_int_not_equal(0, __strcmp(text, not_eqaul_3));	
}

static void strncmp_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char equal[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";

	for(int i = 1; i < strlen(text); i++) {
		assert_int_equal(0, __strncmp(text, equal, i));			
	}

	for(int i = 1; i < strlen(text); i++) {
		assert_int_equal(0, __strncmp(text + i, equal + i, strlen(text) - i));			
	}
}

static void strdup_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char* str;

	for(int i = 0; i < strlen(text); i++) {
		str = __strdup(text + i);
		assert_int_equal(0, strncmp(str, text + i, strlen(text) - i));
	}
}

static void strtol_func() {
	/* Without integner prefix(e.g. 0x, 0b...) */
	char text[1024] = "12345670qwert\0";
	char* non_intger;
	long int rtn = 0;
	int base = 0;

		/* base 0 */
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 8 */
	base = 8;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 10 */
	base = 10;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 16 */
	base = 16;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));



	/* With prefix 0(octet)*/
	memset(text, 0, 1024);
	strncpy(text, "012345670qwert", strlen("012345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 8 */
	base = 8;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 10 */
	base = 10;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 16 */
	base = 16;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));


	/* With prefix 0x(hex)*/
	memset(text, 0, 1024);
	strncpy(text, "0x12345670qwert", strlen("0x12345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));

		/* base 8 */
	base = 8;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

		/* base 10 */
	base = 10;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

		/* base 16 */
	base = 16;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));
}

static void strtoll_func() {
	/* Without integner prefix(e.g. 0x, 0b...) */
	char text[1024] = "12345670qwert\0";
	char* non_intger;
	long long int rtn = 0;
	int base = 0;

		/* base 0 */
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 8 */
	base = 8;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 10 */
	base = 10;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 16 */
	base = 16;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));



	/* With prefix 0(octet)*/
	memset(text, 0, 1024);
	strncpy(text, "012345670qwert", strlen("012345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 8 */
	base = 8;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 10 */
	base = 10;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 16 */
	base = 16;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));


	/* With prefix 0x(hex)*/
	memset(text, 0, 1024);
	strncpy(text, "0x12345670qwert", strlen("0x12345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));

		/* base 8 */
	base = 8;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

		/* base 10 */
	base = 10;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

		/* base 16 */
	base = 16;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));
}


int main() {
	const struct CMUnitTest UnitTest[] = {
//		cmocka_unit_test(memset_func),
//		cmocka_unit_test(memset_sse_func),
//		cmocka_unit_test(memcpy_func),
//		cmocka_unit_test(memcpy_sse_func),
//		cmocka_unit_test(memcmp_func),
//		cmocka_unit_test(memcmp_sse_func),
//		cmocka_unit_test(bzero_func),
//		cmocka_unit_test(strlen_func),
//		cmocka_unit_test(strstr_func),
//		cmocka_unit_test(strchr_func),
//		cmocka_unit_test(strrchr_func),
//		cmocka_unit_test(strcmp_func),
//		cmocka_unit_test(strncmp_func),
//		cmocka_unit_test(strdup_func),
//		cmocka_unit_test(strtol_func),
		cmocka_unit_test(strtoll_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
