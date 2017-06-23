#ifndef __PARAM_H__
#define __PARAM_H__

extern int cpu_start;
extern int cpu_end;

extern long kernel_start_address;	// Kernel Start address
extern char kernel_elf[256];
extern char kernel_args[256];

int param_parse(int argc, char** argv);

#endif /*__PARAM_H__*/
