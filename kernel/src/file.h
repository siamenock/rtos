#ifndef __FILE_H__
#define __FILE_H__

#include <fio.h>

#define FILE_MAX_DESC		0x100

#define FILE_TYPE_NONE		0x00
#define FILE_TYPE_FILE		0x01
#define FILE_TYPE_DIR		0x02

#define FILE_OK			1
#define FILE_EOF		0
#define FILE_ERR_NOSPC		4	/* No space left on device */
#define FILE_ERR_IO		5      	/* I/O error */
#define FILE_ERR_OPENMODE	7	/* Invalid open mode */
#define FILE_ERR_FILENOTEXIST	8	/* File not exist */
#define FILE_ERR_BADFD		9	/* Bad file descriptor */
#define FILE_ERR_BADBUF		10	/* Bad buffer address */
#define FILE_ERR_READONLY	11	/* Read only */
#define FILE_ERR_BADSIZE	12	/* Bad buffer address */
#define FILE_ERR_BUSY		29      /* I/O Busy */
#define FILE_ERR_FTYPE		79	/* Inappropriate file type */
#define FILE_ERR_NOBUFS		105	/* No buffer space available */
#define FILE_ERR_FSDRIVER	106	/* Bad File system driver */

#define FILE_MAX_NAME_LEN	256

typedef struct _FileSystemDriver FileSystemDriver;

typedef struct _File {
	int			type;			///< Type of a file
	int			read_count;
	uint8_t			write_lock;
	FileSystemDriver*	driver; 
	void*			priv;
} File;

int open(const char* file_name, char* flags);
int close(int fd);

ssize_t read(int fd, void* buffer, size_t size);
ssize_t write(int fd, const void* buffer, size_t size);
int read_async(int fd, void* buffer, size_t size, void(*callback)(void* buffer, int size, void* context), void* context);
int write_async(int fd, void* buffer, size_t size, void(*callback)(void* buffer, int size, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2);

int opendir(const char* dir_name);
int readdir(int fd, Dirent* dirent);
void rewinddir(File* dir);
int closedir(int fd);

off_t lseek(int fd, off_t offset, int whence);
int file_size(int fd);

// Device driver API set
typedef struct {
	void* 			buffer;
	size_t 			size;	// Actual size that will be used.
	uint32_t 		sector;
} BufferBlock;
#endif /* __FILE_H__ */
