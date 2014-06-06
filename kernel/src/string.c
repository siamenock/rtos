#include <stdint.h>
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t size) {
	uint64_t* d = dest;
	const uint64_t* s = src;
	
	int count = size / 8;
	while(count--)
		*d++ = *s++;
	
	uint8_t* d2 = (uint8_t*)d;
	uint8_t* s2 = (uint8_t*)s;
	count = size % 8;
	while(count--)
		*d2++ = *s2++;
	
	return dest;
}

void* memset(void* dest, int c, size_t size) {
	uint64_t c8;
	int8_t *p = (void*)&c8;
	
	for(int i = 0; i < 8; i++)
		p[i] = (int8_t)c;
	
	uint64_t* d = dest;
	
	int count = size / 8;
	while(count--)
		*d++ = c8;
	
	uint8_t* d2 = (uint8_t*)d;
	count = size % 8;
	while(count--)
		*d2++ = c;
	
	return dest;
}

void bzero(void* dest, size_t size) {
	uint64_t* d = dest;
	
	int count = size / 8;
	while(count--)
		*d++ = 0;
	
	uint8_t* d2 = (uint8_t*)d;
	count = size % 8;
	while(count--)
		*d2++ = 0;
}

int memcmp(const void* v1, const void* v2, size_t size) {
	const uint64_t* d = v1;
	const uint64_t* s = v2;
	
	int count = size / 8;
	while(count--) {
		if(*d != *s)
			return *s - *d;
		
		s++;
		d++;
	}
	
	const uint8_t* d2 = (uint8_t*)d;
	const uint8_t* s2 = (uint8_t*)s;
	count = size % 8;
	while(count--) {
		if(*d2 != *s2)
			return *s2 - *d2;
		
		s2++;
		d2++;
	}
	
	return 0;
}

void * __attribute__ (( noinline )) __memcpy ( void *dest, const void *src,
					       size_t len ) {
	void *edi = dest;
	const void *esi = src;
	int discard_ecx;

	/* We often do large dword-aligned and dword-length block
	 * moves.  Using movsl rather than movsb speeds these up by
	 * around 32%.
	 */
	__asm__ __volatile__ ( "rep movsl"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len >> 2 )
			       : "memory" );
	__asm__ __volatile__ ( "rep movsb"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len & 3 )
			       : "memory" );
	return dest;
}

/**
 * Copy memory area backwards
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * __attribute__ (( noinline )) __memcpy_reverse ( void *dest,
						       const void *src,
						       size_t len ) {
	void *edi = ( dest + len - 1 );
	const void *esi = ( src + len - 1 );
	int discard_ecx;

	/* Assume memmove() is not performance-critical, and perform a
	 * bytewise copy for simplicity.
	 */
	__asm__ __volatile__ ( "std\n\t"
			       "rep movsb\n\t"
			       "cld\n\t"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ),
				 "2" ( len )
			       : "memory" );
	return dest;
}


/**
 * Copy (possibly overlapping) memory area
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * memmove ( void *dest, const void *src, size_t len ) {

	if ( dest <= src ) {
		return __memcpy ( dest, src, len );
	} else {
		return __memcpy_reverse ( dest, src, len );
	}
}

int strcmp(const char* s, const char* d) {
	while(*s != '\0' && *d != '\0') {
		if(*s != *d)
			return *s - *d;
		
		s++;
		d++;
	}
	
	return *s - *d;
}

int strncmp(const char* s, const char* d, size_t size) {
	if(size == 0)
		return 0;
	
	while(*s != '\0' && *d != '\0') {
		if(*s != *d)
			return *s - *d;
		
		s++;
		d++;
		
		if(--size == 0)
			return 0;
	}
	
	return *s - *d;
}

size_t strlen(const char* s) {
	int i = 0;
	while(s[i] != '\0')
		i++;
	
	return i;
}

char* strstr(const char* haystack, const char* needle) {
	const char* hs = haystack;
	while(*hs != 0) {
		const char* s = hs;
		const char* d = needle;
		while(*d != '\0') {
			if(*s != *d)
				break;
			
			s++;
			d++;
		}
		
		if(*d == '\0')
			return (char*)hs;
		
		hs++;
	}
	
	return NULL;
}

char* strchr(const char* s, int c) {
	char* ch = (char*)s;
	
	while(*ch != c && *ch != '\0')
		ch++;
	
	return *ch == c ? ch : NULL;
}

char* strrchr(const char* s, int c) {
	char* ch = (char*)s;
	
	while(*ch != c && *ch != '\0')
		ch--;
	
	return *ch == c ? ch : NULL;
}

/*
void *__rawmemchr (const void *__s, int __c) {
	char* p = (char*)__s;
	
	while(*p++ != __c);
	
	return p - 1;
}
*/

long int strtol(const char *nptr, char **endptr, int base) {
	if(base == 0) {
		if(nptr[0] == '0') {
			if((nptr[1] == 'x' || nptr[1] == 'X')) {
				base = 16;
				nptr += 2;
			} else {
				base = 8;
				nptr += 1;
			}
		} else {
			base = 10;
		}
	}
	
	long int value = 0;
	
	char* p = (char*)nptr;
	while(1) {
		switch(base) {
			case 16:
				if(*p >= 'a' && *p <= 'f') {
					value = value * base + *(p++) - 'a' + 10;
					continue;
				} else if(*p >= 'A' && *p <= 'F') {
					value = value * base + *(p++) - 'A' + 10;
					continue;
				}
			case 10:
				if(*p >= '0' && *p <= '9') {
					value = value * base + *(p++) - '0';
					continue;
				}
			case 8:
				if(*p >= '0' && *p <= '7') {
					value = value * base + *(p++) - '0';
					continue;
				}
		}
		
		if(endptr != NULL)
			*endptr = p;
		
		return value;
	}
}

long long int strtoll(const char *nptr, char **endptr, int base) {
	if(base == 0) {
		if(nptr[0] == '0') {
			if((nptr[1] == 'x' || nptr[1] == 'X')) {
				base = 16;
				nptr += 2;
			} else {
				base = 8;
				nptr += 1;
			}
		} else {
			base = 10;
		}
	}
	
	long long int value = 0;
	
	char* p = (char*)nptr;
	while(1) {
		switch(base) {
			case 16:
				if(*p >= 'a' && *p <= 'f') {
					value = value * base + *(p++) - 'a' + 10;
					continue;
				} else if(*p >= 'A' && *p <= 'F') {
					value = value * base + *(p++) - 'A' + 10;
					continue;
				}
			case 10:
				if(*p >= '0' && *p <= '9') {
					value = value * base + *(p++) - '0';
					continue;
				}
			case 8:
				if(*p >= '0' && *p <= '7') {
					value = value * base + *(p++) - '0';
					continue;
				}
		}
		
		if(endptr != NULL)
			*endptr = p;
		
		return value;
	}
}

int atoi(const char *nptr) {
	return (int)strtol(nptr, NULL, 10);
}

long atol(const char *nptr) {
	return (long)strtol(nptr, NULL, 10);
}
