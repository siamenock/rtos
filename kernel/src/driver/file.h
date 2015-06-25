#ifndef __DRIVER_FILE_H__
#define __DRIVER_FILE_H__

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define FILE_MAX_NAME_LEN 	256
#define FILE_MAX_DESC		0x100

#define FILE_TYPE_NONE		0x00
#define FILE_TYPE_FILE		0x01
#define FILE_TYPE_DIR		0x02

typedef struct _FileSystemDriver FileSystemDriver;

typedef struct {
	uint32_t 		inode;
	char			name[FILE_MAX_NAME_LEN];
} Dirent;

typedef struct _File {
	uint32_t		inode;
	uint32_t		sector;			///< LBA - sector offset in file system
	size_t			size;			///< Entire file size (byte)
	int			type;
	size_t			index;			///< Cluster index in a file
	size_t			offset;			///< Byte offset in a file
	char			name[FILE_MAX_NAME_LEN];
	FileSystemDriver*	driver; 
	void* 			buf;
	Dirent			dirent;			///< Temporary directory entry (internal use only)
} File;

typedef struct {
	uint32_t		inode;
	size_t			size;
	int 			type;
} Stat;

int open(const char* file_name, char* flags);
int close(int fd);
ssize_t read(int fd, void* buffer, size_t size);
ssize_t write(int fd, const void* buffer, size_t size);
off_t lseek(int fd, off_t offset, int whence);

File* opendir(const char* dir_name);
Dirent* readdir(File* dir); 
void rewinddir(File* dir);
int closedir(File* dir);

int stat(const char* file_name, Stat* stat);

#endif /* __DRIVER_FILE_H__ */
