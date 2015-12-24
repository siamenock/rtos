#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <fio.h>

#define FILE_OK			1
#define FILE_EOF		(-1)
#define FILE_ERR_IO		5      	/* I/O error */
#define FILE_ERR_BADFD		9	/* Bad file descriptor */
#define FILE_ERR_NOSPC		28      /* No space left on device */
#define FILE_ERR_FTYPE		79	/* Inappropriate file type */
#define FILE_ERR_NOBUFS		105	/* No buffer space available */

void file_init();

/**
 * Open and possibly create a file
 */
int file_open(const char* file_name, char* flags, void(*callback)(int fd, void* context), void* context);

/**
 * Close a file descriptor
 */
int file_close(int fd, void(*callback)(int ret, void* context), void* context);

/**
 * Asynchronous read from a file descriptor
 */
int file_read(int fd, void* buffer, size_t size,
	     void(*callback)(void* buffer, size_t size, void* context),
	     void* context);

/**
 * Asynchronous write to a file descriptor
 */
int file_write(int fd, const void* buffer, size_t size,
	      void(*callback)(void* buffer, size_t size, void* context),
	      void* context);

/**
 * Open a directory
 */
int file_opendir(const char* dir_name, void(*callback)(int fd, void* context), void* context);

/**
 * Read a directory
 */
int file_readdir(int fd, Dirent* dir, void(*callback)(Dirent *dir, void* context), void* context); 

/**
 * Close a directory
 */
int file_closedir(int fd, void(*callback)(int ret, void* context), void* context);

#endif /* __FILE_H__ */
