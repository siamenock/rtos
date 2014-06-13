#ifndef __DRIVER_FS_H__
#define __DRIVER_FS_H__

#include <stdint.h>

#define FS_TYPE_NONE	0
#define FS_TYPE_FILE	1
#define FS_TYPE_DIR	2

typedef struct {
	uint64_t	inode;
	uint32_t	type;
	uint32_t	offset;
	char		name[256];
} File;

#endif /* __DRIVER_FS_H__ */
