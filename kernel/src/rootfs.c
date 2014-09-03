#include <stddef.h>
#include <string.h>
#include "gmalloc.h"
#include "driver/bfs.h"
#include "rootfs.h"

#define START	0x10200		// 64KB + 512B
#define SIZE	(512 * 20)

static void* bfs;

static bool rootfs_bfs() {
	if(!bfs) {
		void* addr = (void*)START;
		void* end = addr + SIZE;
		
		while(addr < end) {
			if(*(uint32_t*)addr == BFS_MAGIC) {
				bfs = addr;
				return true;
			}
			
			addr += 512;
		}
		
		return false;
	}
	
	return true;
}

void* rootfs_addr() {
	if(!rootfs_bfs())
		return NULL;
	
	return bfs;
}

uint32_t rootfs_size() {
	if(!rootfs_bfs())
		return 0;
	
	BFS_SuperBlock* super = bfs;
	return super->end + 1;
}

void* rootfs_file(const char* name, uint32_t* size) {
	if(!rootfs_bfs())
		return NULL;
	
	File root, file;
	if(bfs_opendir(bfs, "/", &root) != 0)
		return NULL;
	
	while(bfs_readdir(bfs, &root, &file) == 0) {
		if(file.type == FS_TYPE_FILE && strcmp(file.name, name) == 0) {
			return bfs_mmap(bfs, &file, size);
		}
	}
	
	return NULL;
}

static File root;

bool rootfs_rewind() {
	if(!rootfs_bfs())
		return false;
	
	return bfs_opendir(bfs, "/", &root) != 0;
}

void* rootfs_next(char* name, uint32_t* size) {
	if(!rootfs_bfs())
		return NULL;
	
	File file;
	while(bfs_readdir(bfs, &root, &file) == 0) {
		if(file.type == FS_TYPE_DIR) {
			continue;
		} else if(file.type == FS_TYPE_FILE) {
			memcpy(name, file.name, 14);
			name[14] = '\0';
			
			return bfs_mmap(bfs, &file, 0);
		}
	}
	
	return NULL;
}
