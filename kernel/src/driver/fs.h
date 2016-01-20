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
#define FS_SECTOR_PER_BLOCK	8
#define FS_BLOCK_SIZE		(512 * FS_SECTOR_PER_BLOCK)

#define FS_SEEK_SET		0x00
#define FS_SEEK_CUR		0x01
#define FS_SEEK_END		0x02

#define FS_TYPE_NULL		0x00
#define FS_TYPE_BFS		0x01
#define FS_TYPE_EXT2		0x02

#define FS_MAX_DRIVERS		0x10
#define FS_CACHE_BLOCK		35

#define FS_NUMBER_OF_BLOCKS	FS_CACHE_BLOCK * 4
#define FS_WRITE_BUF_SIZE	2560

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
	int		(*mount)(FileSystemDriver* driver, DiskDriver* disk_driver, uint32_t lba, size_t size);
	int		(*umount)(FileSystemDriver* driver);
	
	File* 		(*open)(FileSystemDriver* driver, File* file, const char* file_name, char* flags);
	/**
	 * @return 0: succeed, -1: IO error
	 */
	int 		(*close)(FileSystemDriver* driver, File* file);
	int		(*read)(DiskDriver* driver, uint32_t lba, size_t sector_count, void* buffer);
	int		(*write)(DiskDriver* driver, uint32_t lba, size_t sector_count, void* buffer);
	int		(*read_async)(DiskDriver* driver, List* blocks, void(*callback)(List* blocks, int count, void* context), void* context);
	int		(*write_async)(DiskDriver* driver, List* blocks, void(*callback)(List* blocks, int count, void* context), void* context);
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
ssize_t fs_read(File* file, void* buffer, size_t size);
ssize_t fs_write(File* file, void* buffer, size_t size);
int fs_read_async(File* file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context);
bool fs_write_async(File* file, void* buffer, size_t size, void(*callback)(void* buffer, size_t len, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2);
int fs_mount(uint32_t disk, uint8_t partition, int type, const char* path);
int fs_umount(const char* path);
bool fs_register(FileSystemDriver* driver);
FileSystemDriver* fs_driver(const char* path);

/**
 * High level disk I/O function which uses disk cache

 */
#endif /* __DRIVER_FS_H__ */
