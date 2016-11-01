#include <stdio.h>
#include <stdlib.h>

int _start() {
  	extern int main(int a, char** argv);
	__asm volatile(
 		"xor %rbp, %rbp\t\n"
 		"mov %rdx, %r9\t\n"
		"mov %rsp, %rsi\t\n"
		"add $0x10, %rsi\t\n"
		"mov 0x8(%rsp), %rdi\t\n"
		"call main");
	exit(0);
}
