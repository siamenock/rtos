#include <stdio.h>
#include <string.h>
#include <zlib.h>

using namespace std;

class Hello {
public:
	void print(const char* title, const char* str) {
		printf("%s: %s\n", title, str);
	}
};

int main(int argc, char** argv) {
	char a[50] = "Hello, world!";
	char b[50];
	char c[50];
	
	Hello hello;
	hello.print("a", a);
	uLong ucompSize = strlen(a) + 1;
	uLong compSize = compressBound(ucompSize);
	
	// Deflate
	compress((Bytef*)b, &compSize, (Bytef*)a, ucompSize);
	
	// Inflate
	uncompress((Bytef*)c, &ucompSize, (Bytef*)b, compSize);
	hello.print("c", c);
	
	return 0;
}
