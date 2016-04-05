#include <stdio.h>
#include "../file.h" 
#include "file_test.h"

A_Test void test_open() {
	// File descriptor index check 
	int fd[256];
	for(int i = 0; i < FILE_MAX_DESC; i++) {
		fd[i] = open("/boot/init.psh", "r");
		assertTrueM("File descriptor index should not be negative number", fd[i] >= 0);
	}
	for(int i = 0; i < FILE_MAX_DESC; i++) {
		close(fd[i]);
	}

	// False flag check
	for(int i = 0; i < 26; i++) {
		char c = 'a' + i;
		// Skip right flags
		if(c == 'r' || c == 'w' || c == 'a')
			continue;	
		fd[0] = open("/boot/init.psh", &c);
		assertFalseM("File open flags should be one of them : 'r', 'w', 'a' ", fd[0] >= 0);
		close(fd[0]);
	}
}

A_Test void test_close() {
	// Open & close repetition
	int fd;
	for(int i = 0; i < 100000; i++) {
		fd = open("/boot/init.psh", "r");	
		assertEqualsM("File close should return FILE_OK (1)", FILE_OK, close(fd));
		close(fd);
	}
}
