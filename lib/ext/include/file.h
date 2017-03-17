#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <fio.h>

#define FILE_EOF		0
#define FILE_OK			1
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
	     void(*callback)(void* buffer, int size, void* context),
	     void* context);

/**
 * Asynchronous write to a file descriptor
 */
int file_write(int fd, const void* buffer, size_t size,
	      void(*callback)(void* buffer, int size, void* context),
	      void* context,
	      void(*sync_callback)(int errno, void* context),
	      void* context2);

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
