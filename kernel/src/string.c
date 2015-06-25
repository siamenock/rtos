#define DONT_MAKE_WRAPPER
#include <_string.h>
#undef DONT_MAKE_WRAPPER
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <malloc.h>

void* __memcpy_chk(void* dest, const void* src, size_t size, size_t bos) {
	return __memcpy(dest, src, size);
}

void *memset(void *s, int c, size_t n) {
	return __memset(s, c, n);
}

void *memcpy(void *dest, const void *src, size_t n) {
	return __memcpy(dest, src, n);
}

void *memmove(void *dest, const void *src, size_t len) {
	return __memmove(dest, src, len);
}

int memcmp(const void* v1, const void* v2, size_t size) {
	return __memcmp(v1, v2, size);
}

void bzero(void* dest, size_t size) {
	return __bzero(dest, size);
}

size_t strlen(const char* s) {
	return __strlen(s);
}

char* strstr(const char* haystack, const char* needle) {
	return __strstr(haystack, needle);
}

char* strchr(const char* s, int c) {
	return __strchr(s, c);
}

char* strrchr(const char* s, int c) {
	return __strrchr(s, c);
}

int strcmp(const char *s, const char *d) {
	return __strcmp(s, d);
}

int strncmp(const char* s, const char* d, size_t size) {
	return __strncmp(s, d, size);
}

char* strdup(const char* source) {
	return __strdup(source);
}

long int strtol(const char *nptr, char **endptr, int base) {
	return __strtol(nptr, endptr, base);
}

long long int strtoll(const char *nptr, char **endptr, int base) {
	return __strtoll(nptr, endptr, base);
}

char* strcpy(char *dest, const char *src) {
	return __memcpy(dest, src, strlen(src) + 1);
}

