#include <stdio.h>
#include <string.h>
#include <zlib.h>

int main(int argc, char** argv) {
	char a[50] = "Hello, world!";
	char b[50];
	char c[50];
	
	uLong ucompSize = strlen(a) + 1;
	uLong compSize = compressBound(ucompSize);
	
	// Deflate
	compress((Bytef*)b, &compSize, (Bytef*)a, ucompSize);
	
	// Inflate
	uncompress((Bytef*)c, &ucompSize, (Bytef*)b, compSize);
	printf("%s\n", c);
	
	return 0;
}
