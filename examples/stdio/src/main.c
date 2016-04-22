#include <stdio.h>
#include <thread.h>
#include <string.h>
#include <readline.h>
void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	//char name[128] = { 0, };
	char *name;
	printf("Input your name: ");
	//fflush(stdout);
	while(1) {
		//int len = scanf("%s", name);
		name = readline();
		printf("readline()\n");
		if(name){
			printf("%s\n", name);
		}
		int len = strlen(name);
		printf("%s\n", name);
		if(len > 0) {
			printf("%d out> Hello %s from thread %d\n", len, name, thread_id());
			//fprintf(stdout, "%d out> Hello %s from thread %d\n", len, name, thread_id());
			//fprintf(stderr, "%d err> Hello %s from thread %d\n", len, name, thread_id());
			fflush(stdout);
		}

	}
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
