#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: rewrite [image] [loader]\n");
		return 0;
	}
	
	FILE* image = fopen(argv[1], "r+");
	FILE* loader = fopen(argv[2], "r+");

	if(image == NULL || loader == NULL) {
		printf("Cannot open file: %s\n", argv[1]);
		return 1;
	}
	
	struct stat state;
	if(stat(argv[2], &state) != 0) {
		printf("Cannot get state of file: %s\n", argv[1]);
		return 2;
	}

	uint32_t total = state.st_size / 512;
	if(total > UINT16_MAX) {
		printf("Total size exceed: %d\n", total);
		return 3;
	}
	
	if(fseek(image, 5L, SEEK_SET) != 0) {
		printf("Cannot find the rewrite position.\n");
		return 5;
	}
	
	uint16_t u16 = total;
	if(fwrite(&u16, 1, sizeof(uint16_t), image) != sizeof(uint16_t)) {
		printf("Cannot write data to the file: %s\n", argv[1]);
		return 6;
	}
	
	if((fclose(image)) != 0) {
		printf("Cannot close the file properly: %s\n", argv[1]);
		return 7;
	}

	if((fclose(loader)) != 0) {
		printf("Cannot close the file properly: %s\n", argv[2]);
		return 8;
	}
	
	return 0;
}
