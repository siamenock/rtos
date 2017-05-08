#ifndef __DRIVER_FS_H__
#define __DRIVER_FS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <util/cache.h>
#include <fio.h>

#include "../file.h"
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
#define FS_TYPE_FAT		0x03

#define FS_MAX_DRIVERS		0x10
#ifdef _KERNEL_
#define FS_CACHE_BLOCK		50
#else
#define FS_CACHE_BLOCK		30
#endif

#define FS_NUMBER_OF_BLOCKS	FS_CACHE_BLOCK * 4
#define FS_WRITE_BUF_SIZE	200

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
	int		(*mount)(FileSystemDriver* driver, DiskDriver* disk_driver, uint32_t lba);
	int		(*umount)(FileSystemDriver* driver);
	
	int 		(*open)(FileSystemDriver* driver, const char* file_name, char* flags, void** priv);
	/**
	 * @return 0: succeed, -1: IO error
	 */
	int 		(*close)(FileSystemDriver* driver, void* file);
	int		(*read)(FileSystemDriver* driver, void* file, void* buffer, size_t size);
	int		(*write)(FileSystemDriver* driver, void* file, void* buffer, size_t size);
	int		(*read_async)(FileSystemDriver* driver, void* file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context);
	int		(*write_async)(FileSystemDriver* driver, void* file, void* buffer, size_t size, void(*callback)(void* buffer, int len, void* context), void* context, void(*sync_callback)(int errno, void* context), void* context2);
	off_t 		(*lseek)(FileSystemDriver* driver, void* file, off_t offset, int whence);
	
	void* 		(*opendir)(FileSystemDriver* driver, const char* dir_name);
	int	  	(*readdir)(FileSystemDriver* driver, void* dir, Dirent* dirent);
	void 		(*rewinddir)(FileSystemDriver* driver, void* dir);
	int 		(*closedir)(FileSystemDriver* driver, void* dir);
	int		(*file_size)(void* file);
	bool		(*check_sync)(void* file);
	
	/* Mount information */
	DiskDriver* 	driver;
	Cache*		cache;
	char*		path;		// Mounting point
	void*		priv;
} FileSystemDriver;	// BFSDriver, EXT2Driver, ...

bool fs_init();
ssize_t fs_read(File* file, void* buffer, size_t size);
ssize_t fs_write(File* file, void* buffer, size_t size);
int fs_read_async(File* file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context);
int fs_write_async(File* file, void* buffer, size_t size, void(*callback)(void* buffer, int len, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2);
int fs_mount(uint32_t disk, uint8_t partition, int type, const char* path);
int fs_umount(const char* path);
bool fs_register(FileSystemDriver* driver);
FileSystemDriver* fs_driver(const char* path);

/**
 * High level disk I/O function which uses disk cache

 */
#endif /* __DRIVER_FS_H__ */
