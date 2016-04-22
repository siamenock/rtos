#include <stdio.h>
#include <string.h>
#include <zlib.h>

int main(int argc, char** argv) {
	char a[100] = "Hello, world! Hello, world! Hello, world! Hello, world!";
	char b[100] = { 0, };
	char c[100] = { 0, };
	
	uLong ucompSize = strlen(a);
	uLong compSize = compressBound(ucompSize);
	
	printf("Original:     [%ld] %s\n", ucompSize, a);
	
	// Deflate
	compress((Bytef*)b, &compSize, (Bytef*)a, ucompSize);
	printf("Compressed:   [%ld] ", compSize);
	for(int i = 0; i < compSize; i++) {
		printf("%02x ", b[i] & 0xff);
	}
	printf("\n");
	
	// Inflate
	uncompress((Bytef*)c, &ucompSize, (Bytef*)b, compSize);
	printf("Uncompressed: [%ld] %s\n", ucompSize, c);
	
	return 0;
}
