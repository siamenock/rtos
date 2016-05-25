#include <stdio.h>
#include <readline.h>

int main(int argc, char** argv) {
	while(1) {
		char* line = readline();
		
		if(line != NULL) {
			printf("Hello %s!\n", line);
		} 

	}
	
	return 0;
}
