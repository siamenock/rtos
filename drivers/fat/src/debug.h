/*
 * debug.h
 *
 * debug functions
 * head file
 * 
 * knightray@gmail.com
 * 10/28 2008
 */
#ifndef _DEBUG_H
#define _DEBUG_H

#include "comdef.h"

/* Start of GurumNetworks modification
#define DBG_FAT
#define DBG_DIRENT
#define DBG_FILE
#define DBG_DIR
#define DBG_CACHE
#define DBG_INITFS 
End of GurumNetworks modification */

#define DBG 	printf
#define WARN	printf
#define ERR	printf
#define INFO 	printf

#define ASSERT(f)	if(!(f)) { \
						printf("ASSERT failed at %s():%d\n", __FUNCTION__, __LINE__); \
						exit(0); \
					}

void print_sector(ubyte * secbuf, ubyte bychar);
void nulldbg(char * fmt, ...);

#endif
