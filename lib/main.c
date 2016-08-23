#include <stdio.h>

void* malloc(size_t size) {
	printf("Hello\n");
}

int main(int argc, const char *argv[]) {
	malloc(10);
	return 0;
}
