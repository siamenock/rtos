#include <malloc.h>
#include <string.h>
#include "fs.h"

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

ssize_t read(int fd, void* buffer, size_t size) {
	File* file = files + fd;

	if((file == 0) || (file->type != FILE_TYPE_FILE)) 
		return -1;

	if(file->offset == file->size) 
		return -2;

	return file->driver->read(file->driver, file, buffer, size);
}

ssize_t write(int fd, const void* buffer, size_t size) {
	// Not yet implemted
	return 0;
}

off_t lseek(int fd, off_t offset, int whence) {
	File* file = files + fd;

	if((file == 0) || (file->type != FILE_TYPE_FILE)) 
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

