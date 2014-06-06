#include <stdio.h>
#include <net/ether.h>
#include "dummy.h"

// Dummy code not to omitting functions
void dummy_init() {
	if((void*)dummy_init > (void*)0x1000)	// Logically always true
		return;
	
	read_u8(NULL, NULL);
	read_u16(NULL, NULL);
	read_u32(NULL, NULL);
	read_u48(NULL, NULL);
	read_u64(NULL, NULL);
	write_u8(NULL, 0, NULL);
	write_u16(NULL, 0, NULL);
	write_u32(NULL, 0, NULL);
	write_u48(NULL, 0, NULL);
	write_u64(NULL, 0, NULL);
}
