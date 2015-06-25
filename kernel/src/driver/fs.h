#ifndef __DRIVER_FS_H__
#define __DRIVER_FS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <util/cache.h>
#include "file.h"
#include "disk.h"

#define FS_SECTOR_SIZE		512
#define FS_SECTOR_PER_CLUSTER	8
#define FS_CLUSTER_SIZE		(512 * 8)   

#define FS_SEEK_SET		0x00
#define FS_SEEK_CUR		0x01
#define FS_SEEK_END		0x02

#define FS_TYPE_NULL		0x00
#define FS_TYPE_BFS		0x01
#define FS_TYPE_EXT2		0x02

#define FS_MAX_DRIVERS		0x10

#define FS_DEFAULT_CACHE_SIZE	8192

/* Available mounting devices for each driver */
#define FS_MOUNTING_POINTS	8 

typedef struct _FileSystemDriver FileSystemDriver;

typedef struct _FileSystemDriver {
	/* Driver information */
	int 		type;		///< BFS, EXT2, ...

	// Hgih level file operation - file system specific 
	/**
	 * @return 0: OK, -1: I/O error, -2: Illegal file system
	 */
	int		(*mount)(FileSystemDriver* driver, DiskDriver* disk_driver);
	int		(*umount)(FileSystemDriver* driver);
	
	File* 		(*open)(FileSystemDriver* driver, File* file, const char* file_name, char* flags);
	/**
	 * @return 0: succeed, -1: IO error
	 */
	int 		(*close)(FileSystemDriver* driver, File* file);
	ssize_t		(*read)(FileSystemDriver* driver, File* file, void* buffer, size_t size);
	ssize_t 	(*write)(FileSystemDriver* driver, File* file, const void* buffer, size_t size);
	off_t 		(*lseek)(FileSystemDriver* driver, File* file, off_t offset, int whence);
	
	File* 		(*opendir)(FileSystemDriver* driver, File* dir, const char* dir_name);
	Dirent* 	(*readdir)(FileSystemDriver* driver, File* dir); 
	void 		(*rewinddir)(FileSystemDriver* driver, File* dir);
	int 		(*closedir)(FileSystemDriver* driver, File* dir);
	
	/* Mount information */
	DiskDriver* 	driver;
	Cache*		cache;
	void*		priv;
} FileSystemDriver;	// BFSDriver, EXT2Driver, ...

bool fs_init();
int fs_mount(int type, uint32_t device, const char* path);
int fs_umount(const char* path);
bool fs_register(const FileSystemDriver* driver);
FileSystemDriver* fs_driver(const char* path);

/**
 * High level disk I/O function which uses disk cache
 */
ssize_t fs_read(FileSystemDriver* driver, uint32_t lba, size_t sector_count, void* buf);
ssize_t fs_write(FileSystemDriver* driver, uint32_t lba, size_t sector_count, void* buf);

#endif /* __DRIVER_FS_H__ */
