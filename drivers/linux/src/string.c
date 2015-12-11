#include <linux/string.h>

void *memset(void *s, int c, size_t count) {
	char *xs = s;

	while (count--)
		*xs++ = c;
	return s;
}

void *memcpy(void *dest, const void *src, size_t count) {
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;
	return dest;
}

size_t strlen(const char *s) {
	int i = 0;
	while(s[i] != '\0')
		i++;

	return i;
}

char *strcpy(char *dest, const char *src) {
	return memcpy(dest, src, strlen(src) + 1);
}

char *strncpy(char *dest, const char *src, size_t n) {
	size_t i;
	for(i = 0; i < n && src[i] != '\0'; i++)
		dest[i] = src[i];
	
	for(; i < n; i++)
		dest[i] = '\0';
	
	return dest;
}
