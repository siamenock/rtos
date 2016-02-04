#ifndef __BFS_H__
#define __BFS_H__

#include <stdbool.h>
#include "fs.h"

#define BFS_MAGIC	0x1badface
#define BFS_BLOCK_SIZE	512
#define BFS_TYPE_FILE	1
#define BFS_TYPE_DIR	2
#define BFS_NAME_LEN 	14
#define BFS_MAX_DRIVERS 0x08

#define BFS_MAX_NAME_LEN	256

typedef struct {
	uint32_t		inode;
	uint32_t		sector;			///< LBA - sector offset in file system
	size_t			size;			///< Entire file size (byte)
	size_t			offset;			///< Byte offset in a file
	char			name[BFS_MAX_NAME_LEN];
	void*			buf;
	Dirent			dirent;			///< Temporary directory entry (internal use only)
} BFSFile;

bool bfs_init();

extern FileSystemDriver bfs_driver;

#endif /* __BFS_H__ */
