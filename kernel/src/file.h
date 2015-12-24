#ifndef __FILE_H__
#define __FILE_H__

#include <file.h>
#define FILE_MAX_DESC		0x100

#define FILE_TYPE_NONE		0x00
#define FILE_TYPE_FILE		0x01
#define FILE_TYPE_DIR		0x02

#define FILE_OK			1
#define FILE_EOF		(-1)
#define FILE_ERR_IO		5      	/* I/O error */
#define FILE_ERR_BADFD		9	/* Bad file descriptor */
#define FILE_ERR_NOSPC		28      /* No space left on device */
#define FILE_ERR_FTYPE		79	/* Inappropriate file type */
#define FILE_ERR_NOBUFS		105	/* No buffer space available */

#define FILE_MAX_NAME_LEN	256

typedef struct _FileSystemDriver FileSystemDriver;

typedef struct _File {
	uint32_t		inode;
	uint32_t		sector;			///< LBA - sector offset in file system
	size_t			size;			///< Entire file size (byte)
	int			type;			///< Type of a file
	size_t			offset;			///< Byte offset in a file
	char			name[FILE_MAX_NAME_LEN];
	void*			buf;
	Dirent			dirent;			///< Temporary directory entry (internal use only)
	FileSystemDriver*	driver; 
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
int read_async(int fd, void* buffer, size_t size, void(*callback)(void* buffer, size_t size, void* context), void* context);
int write_async(int fd, void* buffer, size_t size, void(*callback)(void* buffer, size_t size, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2);
off_t lseek(int fd, off_t offset, int whence);

int opendir(const char* dir_name);
Dirent* readdir(int fd); 
void rewinddir(File* dir);
int closedir(int fd);

int stat(const char* file_name, Stat* stat);

// Device driver API set
typedef struct {
	void* 			buffer;
	size_t 			size;	// Actual size that will be used.
	uint32_t 		sector;
} BufferBlock;

#endif /* __FILE_H__ */
