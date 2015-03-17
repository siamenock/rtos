#include <asm/uaccess.h>
#include <linux/string.h>

unsigned long copy_from_user(void *to, const void *from, unsigned long n)
{
	memcpy(to, from, n);
	return 0;
//	if (access_ok(VERIFY_READ, from, n))
//		n = __copy_from_user(to, from, n);
//	else
//		memset(to, 0, n);
//	return n;
}

unsigned long copy_to_user(void *to, const void *from, unsigned long n)
{
	memcpy(to, from, n);
	return 0;
//	if (access_ok(VERIFY_WRITE, to, n))
//		n = __copy_to_user(to, from, n);
//	return n;
}
