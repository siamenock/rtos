#include <linux/err.h>

void* ERR_PTR(long error)
{
	return (void *) error;
}

long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

bool IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

void* ERR_CAST( const void *ptr)
{
	/* cast away the const */
	return (void *) ptr;
}

int  PTR_ERR_OR_ZERO(const void *ptr)
{
	if (IS_ERR(ptr))
		return PTR_ERR(ptr);
	else
		return 0;
}

