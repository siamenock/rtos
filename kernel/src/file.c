#include <malloc.h>
#include <gmalloc.h>
#include <string.h>
#include "driver/fs.h"
#include "cpu.h"
#include <util/event.h>


typedef struct {
	void(*callback)(void* buffer, size_t len, void* context);
	void*			context;
	void*			buffer;
} ReadContext;

static File files[FILE_MAX_DESC]; 

static void* alloc_descriptor(void) {
	for(int i = 0; i < FILE_MAX_DESC; i++) {
		if(files[i].type == FILE_TYPE_NONE) {
			files[i].type = FILE_TYPE_FILE;
			return &files[i];
		}
	}
	
	return NULL;
}

static void free_descriptor(File* descriptor) {
	descriptor->type = FILE_TYPE_NONE;
}

/**
 * Path is absolute path with file name e.g. /boot/kernel.bin
 */
int open(const char* path, char* flags) {
	// Name validation check
	int len = strlen(path);
	int result;
	
	if(flags[0] != 'r') { // Reading is only available now
		result = -1;
		goto failed;
	}

	if((len > FILE_MAX_NAME_LEN - 1) || (len == 0)) {
		result = -2;
		goto failed;
	}

	File* file = alloc_descriptor();
	if(!file) {
		result = -3;
		goto failed;
	}

	// Check whether format of path is valid
	if(path[0] != '/') {
		result = -4;
		goto failed;
	}

	file->driver = fs_driver(path);
	if(!file->driver) {
		result = -5;
		goto failed;
	}

	const char* get_file_name(const char* path) {
		char* base = strrchr(path, '/');
		return base ? base + 1 : path;
	}

	if(file->driver->open(file->driver, file, get_file_name(path), flags) == NULL) {
		result = -6;
		goto failed;
	}

	// Return index of file descriptor
	return file - files; 

failed:
	if(file)
		free_descriptor(file);

	return result;
}

int close(int fd) {
	File* file = files + fd;
	if((file == 0) || (file->type != FILE_TYPE_FILE)) 
		return -1;

	if(file->driver->close(file->driver, file) < 0)
		return -2;

	file->driver = NULL;

	free_descriptor(file);

	return 0;
}

static int file_check(File* file) {
	// Check if file is NULL or file type is invaild
	if(file == NULL || file->type != FILE_TYPE_FILE) {
		return -FILE_ERR_FTYPE;
	}

	// Check if offset indicates end of file
	if(file->offset >= file->size) {
		return FILE_EOF;
	}

	return FILE_OK;
}

ssize_t write(int fd, const void* buffer, size_t size) {
	File* file = files + fd;
	
	if(fd < 0 || fd >= FILE_MAX_DESC) {
		return FILE_ERR_BADFD;
	}

	int err = file_check(file);
	if(err != FILE_OK)
		return err;

	return fs_write(file, (void*)buffer, size);
}

ssize_t read(int fd, void* buffer, size_t size) {
	File* file = files + fd;

	if(fd < 0 || fd >= FILE_MAX_DESC) {
		return FILE_ERR_BADFD;
	}

	int err = file_check(file);
	if(err != FILE_OK)
		return err;

	return fs_read(file, buffer, size);
}

static bool read_callback(List* blocks, int success, void* context) {
	ReadContext* read_context = context;
	
	int len = 0;
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		BufferBlock* block = list_iterator_next(&iter);
		memcpy((uint32_t*)((uint8_t*)read_context->buffer + len), block->buffer, block->size);
		list_iterator_remove(&iter);
		len += block->size;
		free(block);
	}

	if(success < 0)
		len = success;

	read_context->callback(read_context->buffer, len, read_context->context);
	free(read_context);
	
	return true;
}

int read_async(int fd, void* buffer, size_t size, void(*callback)(void* buffer, size_t size, void* context), void* context) {
	File* file = files + fd;

	if(fd < 0 || fd >= FILE_MAX_DESC) {
		callback(buffer, FILE_ERR_BADFD, context);
		return FILE_ERR_BADFD;
	}

	int err = file_check(file);
	if(err != FILE_OK) {
		callback(buffer, err, context);
		return err;
	}

	ReadContext* read_context = malloc(sizeof(ReadContext));
	read_context->callback = callback;
	read_context->context = context;
	read_context->buffer = buffer;

	if(fs_read_async(file, size, read_callback, read_context) < 0) {
		// If no buffer space available
		callback(buffer, -FILE_ERR_NOBUFS, context);
		free(read_context);
		return -FILE_ERR_NOBUFS;
	}

	return 0;
}

int write_async(int fd, void* buffer, size_t size, void(*callback)(void* buffer, size_t size, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2) {
	File* file = files + fd;

	if(fd < 0 || fd >= FILE_MAX_DESC) {
		callback(buffer, FILE_ERR_BADFD, context);
		return FILE_ERR_BADFD;
	}

	int err = file_check(file);
	if(err != FILE_OK) {
		callback(buffer, err, context);
		return err;
	}

	fs_write_async(file, buffer, size, callback, context, sync_callback, context2);

	return 0;
}

off_t lseek(int fd, off_t offset, int whence) {
	File* file = files + fd;

	if(file == 0 || file->type != FILE_TYPE_FILE) 
		return 0;

	return file->driver->lseek(file->driver, file, offset, whence);
}

File* opendir(const char* dir_name) {
	File* file = alloc_descriptor();

	if(file == NULL)
		return NULL;

	file->driver = fs_driver(dir_name);
	if(!file->driver) {
		free_descriptor(file);
		return NULL;
	}

	return file->driver->opendir(file->driver, file, dir_name);
}

Dirent* readdir(File* dir) {
	if((dir == NULL) || (dir->type != FILE_TYPE_DIR)) 
		return NULL;

	if(dir->offset == dir->size) 
		return NULL;
	
	return dir->driver->readdir(dir->driver, dir);
}

void rewinddir(File* dir) {
	// Not implemented yet
}

int closedir(File* dir) {
	if((dir == 0) || (dir->type != FILE_TYPE_DIR)) 
		return -1;

	if(dir->driver->closedir(dir->driver, dir) < 0)
		return -2;

	free_descriptor(dir);

	return 0;
}

int stat(const char* file_name, Stat* stat) {
	// Name validation check
	int len = strlen(file_name);
	
	if((len > FILE_MAX_NAME_LEN - 1) || (len == 0)) 
		return -1;

	File* descriptor = files;

	for(int i = 0; i < FILE_MAX_DESC; i++) {
		if(descriptor->type != FILE_TYPE_NONE) {
			if(strncmp(descriptor->name , file_name, len) == 0) {
				// TODO: Add more status 
				stat->inode = descriptor->inode;
				stat->size = descriptor->size;
				stat->type = descriptor->type;

				return 0;
			}
		}

		descriptor++;
	}

	return -2;
}

