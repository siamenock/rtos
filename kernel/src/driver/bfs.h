#ifndef __BFS_H__
#define __BFS_H__

#include "fs.h"

#define BFS_MAGIC	0x1badface

#define BFS_BLOCK_SIZE	512

#define BFS_TYPE_FILE	1
#define BFS_TYPE_DIR	2

typedef struct {
	uint32_t	magic;
	uint32_t	start;
	uint32_t	end;
	uint32_t	from;
	uint32_t	to;
	int32_t		bfrom;
	int32_t		bto;
	char		fsname[6];
	char		volume[6];
	uint32_t	padding[118];
} __attribute__((packed)) BFS_SuperBlock;

typedef struct {
	uint16_t	inode;
	uint16_t	unused;
	uint32_t	first;
	uint32_t	last;
	uint32_t	eof;
	uint32_t	type;
	uint32_t	mode;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	nlink;
	uint32_t	atime;
	uint32_t	mtime;
	uint32_t	ctime;
	uint32_t	padding[4];
} __attribute__((packed)) BFS_Inode;

typedef struct {
	uint16_t	inode;
	uint8_t		name[14];
} __attribute__((packed)) BFS_Dir;

int bfs_opendir(void* bfs, const char* name, File* file);
int bfs_readdir(void* bfs, File* dir, File* file);
void bfs_rewinddir(void* bfs, File* dir);
void bfs_closedir(void* bfs, File* dir);
void* bfs_mmap(void* bfs, File* file, uint32_t* size);

#endif /* __BFS_H__ */
