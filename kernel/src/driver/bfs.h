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

extern FileSystemDriver bfs_driver;

#endif /* __BFS_H__ */
