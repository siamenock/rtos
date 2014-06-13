#include "bfs.h"

int bfs_opendir(void* bfs, const char* name, File* file) {
	BFS_SuperBlock* super = bfs;
	if(super->magic != BFS_MAGIC)
		return -1;	// Invalid BFS file system
	
	if(name[0] != '/' || name[1] != '\0')
		return -2;	// There is no such directory
	
	BFS_Inode* inodes = (BFS_Inode*)(bfs + sizeof(BFS_SuperBlock) - sizeof(BFS_Inode) * 2);
	file->inode = inodes[2].inode;
	file->type = FS_TYPE_DIR;
	file->offset = 0;
	file->name[0] = '/';
	file->name[1] = '\0';
	
	return 0;
}

int bfs_readdir(void* bfs, File* dir, File* file) {
	BFS_Inode* inodes = (BFS_Inode*)(bfs + sizeof(BFS_SuperBlock) - sizeof(BFS_Inode) * 2);
	BFS_Inode* inode_dir = &inodes[dir->inode];
	
	BFS_Dir* dir2 = bfs + BFS_BLOCK_SIZE * inode_dir->first + dir->offset;
	dir->offset += sizeof(BFS_Dir);
	
	BFS_Inode* inode_file = &inodes[dir2->inode];
	
	switch(inode_file->type) {
		case BFS_TYPE_FILE:
			file->type = FS_TYPE_FILE;
			goto set_file;
		case BFS_TYPE_DIR:
			file->type = FS_TYPE_DIR;
set_file:
			file->inode = dir2->inode;
			file->offset = 0;
			for(int i = 0; i < 14; i++)
				file->name[i] = dir2->name[i];
			file->name[14] = 0;
			return 0;
		default:
			return -1;	// End of entities
	}
}

void bfs_rewinddir(void* bfs, File* dir) {
	dir->offset = 0;
}

void bfs_closedir(void* bfs, File* dir) {
	// Do nothing
}

void* bfs_mmap(void* bfs, File* file, uint32_t* size) {
	BFS_Inode* inodes = (BFS_Inode*)(bfs + sizeof(BFS_SuperBlock) - sizeof(BFS_Inode) * 2);
	BFS_Inode* inode = &inodes[file->inode];
	
	if(inode->type != BFS_TYPE_FILE)
		return (void*)0;	// There is no such file
	
	if(size)
		*size = inode->eof - inode->first * BFS_BLOCK_SIZE + 1;
	
	return bfs + inode->first * BFS_BLOCK_SIZE;
}

#if TEST
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

int main(int argc, char** argv) {
	int fd = open(argv[1], O_RDONLY);
	struct stat state;
	stat(argv[1], &state);
	
	void* bfs = mmap(NULL, state.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	File root;
	int result = bfs_opendir(bfs, "/", &root);
	printf("opendir: %d inode: %ld, type: %d, name: %s\n", result, root.inode, root.type, root.name);
	File file;
	for(int i = 0; i < 3; i++) {
		printf("Rewind %d\n", i);
		while(bfs_readdir(bfs, &root, &file) == 0) {
			printf("\tinode: %ld, type: %d, name: %s\n", file.inode, file.type, file.name);
			if(file.type == FS_TYPE_FILE) {
				uint32_t size;
				void* mmap = bfs_mmap(bfs, &file, &size);
				printf("\t\tmmap: %p %ld\n", mmap, size);
				printf("\t\t");
				for(int j = 0; j < 10; j++)
					printf("%02x ", ((uint8_t*)mmap)[j]);
				printf("\n");
				printf("\t\t");
				for(int j = 0; j < 10; j++)
					printf("%02x ", ((uint8_t*)mmap + size - 10)[j]);
				printf("\n");
			}
		}
		bfs_rewinddir(bfs, &root);
	}
	
	close(fd);
	
	return 0;
}
#endif
