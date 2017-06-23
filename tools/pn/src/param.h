#ifndef __PARAM_H__
#define __PARAM_H__

int cpu_start;
int cpu_end;

long kernel_start_address;	// Kernel Start address
long rd_start_address; 	// Ramdisk Start address
char kernel_elf[256];
char kernel_args[256];

int param_parse(char* params);

#endif /*__PARAM_H__*/
