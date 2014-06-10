#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
	if(argc < 1) {
		printf("Usage: rewrite [image]\n");
		return 0;
	}
	
	FILE* file = fopen(argv[1], "r+");
	if(file == NULL) {
		printf("Cannot open file: %s\n", argv[1]);
		return 1;
	}
	
	struct stat state;
	if(stat(argv[1], &state) != 0) {
		printf("Cannot get state of file: %s\n", argv[1]);
		return 1;
	}
	
	uint32_t total = (state.st_size + 511) / 512 - 1;
	
	if(total > UINT16_MAX) {
		printf("Total size exceed: %d\n", total);
		return 5;
	}
	
	if(fseek(file, 5L, SEEK_SET) != 0) {
		printf("Cannot find the rewrite position.\n");
		return 2;
	}
	
	uint16_t u16 = total;
	if(fwrite(&u16, 1, sizeof(uint16_t), file) != sizeof(uint16_t)) {
		printf("Cannot write data to the file: %s\n", argv[1]);
		return 3;
	}
	
	if(fclose(file) != 0) {
		printf("Cannot close the file properly: %s\n", argv[1]);
		return 4;
	}
	
	return 0;
}
