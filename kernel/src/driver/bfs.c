#include <malloc.h>
#include <util/event.h>
#include "bfs.h"
#include "string.h"
#include "../gmalloc.h"
#include "../cpu.h"

typedef struct {
	uint16_t	inode;
	uint8_t 	name[BFS_NAME_LEN];
} __attribute__((packed)) BFSDir;

typedef struct {
	uint32_t	magic;
	uint32_t	start;
	uint32_t	end;
	uint32_t	from;
	uint32_t	to;
	int32_t		bfrom;
	int32_t		bto;
	char		fsname[6];
	char		volume[6];
	uint32_t	padding[118];
} __attribute__((packed)) BFSSuperBlock;

typedef struct {
	uint16_t	inode;
	uint16_t	unused;
	uint32_t	first;
	uint32_t	last;
	uint32_t	eof;
	uint32_t	type;
	uint32_t	mode;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	nlink;
	uint32_t	atime;
	uint32_t	mtime;
	uint32_t	ctime;
	uint32_t	padding[4];
} __attribute__((packed)) BFSInode;

/**
 * Private structure for BFS. All of addresses below are LBA. 
 */
typedef struct {
	uint64_t 	reserved_addr;		///< Super block address
	uint32_t	reserved_size;		///< Sector count of super block 
	uint32_t	link_table_addr;	///< Inode table address
	uint32_t	link_table_size;	///< Sector count of inode table
	uint32_t	data_addr;		///< Data block address
	uint32_t	total_cluster_count;	///< Total cluster count of BFS
} BFSPriv;


/** 
 * PacketNgin doesn't have partition table in MBR. So we have to find file system first.
 */

// TODO: Checking superblock only, not investigate whole disks
static int bfs_mount(FileSystemDriver* fs_driver, DiskDriver* disk_driver, uint32_t lba, size_t size) {
	// Size of loader is now about 60kB. Investigate til 200kB (50 Clusters)
	BFSSuperBlock* sb;

	void* temp = gmalloc(FS_BLOCK_SIZE);

	for(int i = 0; i < 50; i++) {
		if(disk_driver->read(disk_driver, lba, 8, temp) != 0) {
			sb = (BFSSuperBlock*)temp;
			if(sb->magic == BFS_MAGIC) {
				// Read 1 more sector for 8th sector
				if(disk_driver->read(disk_driver, lba + 1, 1, temp) != 0) {
					// Create private data structure
					fs_driver->priv = malloc(sizeof(BFSPriv));
					BFSPriv* priv = fs_driver->priv;

					priv->reserved_addr = lba;
					priv->reserved_size = 1;

					priv->link_table_addr = lba+ 1;
					BFSInode* inode = (BFSInode*)temp;
					priv->link_table_size = inode->first /* Start of Data */ 
						- 1 /* Super block */;

					priv->data_addr = lba + inode->first;

					int size = sb->end - sb->start; // Total size (bytes)
					priv->total_cluster_count = (size - 4095) / 4096;

					// Disk driver attachment
					fs_driver->driver = disk_driver;

					return 0;
				}
			}

			sb = (BFSSuperBlock*)((uintptr_t)sb + 512);
		}
	}

	return -1; 
}

static int bfs_umount(FileSystemDriver* driver) {
	// Destory private data structure 
	if(!driver->priv)
		return -1;

	free(driver->priv);
	driver->priv = NULL;

	return 0;
}

static BFSInode* get_inode_entry(FileSystemDriver* driver, uint32_t idx) {
	BFSPriv* priv = driver->priv;
	DiskDriver* disk_driver = driver->driver;

	// NOTE: Assume that inode table is smaller than one cluster
	void* temp = cache_get(driver->cache, (void*)(uintptr_t)(priv->link_table_addr));
	if(!temp) {
		temp = gmalloc(FS_BLOCK_SIZE);
		if(disk_driver->read(disk_driver, priv->link_table_addr, FS_SECTOR_PER_BLOCK, temp) <= 0) {
			printf("Reading root directory failed\n");
			gfree(temp);
			return NULL;
		}
		if(!cache_set(driver->cache, (void*)(uintptr_t)(priv->link_table_addr), temp)) {
			printf("cache setting error\n");
			gfree(temp);
			return NULL;
		}
	}

	BFSInode* inode_entry = (BFSInode*)temp;

	int entry_count = (priv->link_table_size * FS_SECTOR_SIZE) / sizeof(BFSInode);

	for(int i = 0; i < entry_count; i++) {
		if(inode_entry->inode == idx) {
			return inode_entry;
		} 

		inode_entry++;
	}
		
	return NULL;
}

// Find directory entry which corresponds to file name from root directory
static int find_directory_entry(FileSystemDriver* driver, const char* name, File* file) { 
	BFSPriv* priv = driver->priv;
	DiskDriver* disk_driver = driver->driver;

	// Read root directory
	// NOTE: Assume that root directory is smaller than one cluster
	void* temp = cache_get(driver->cache, (void*)(uintptr_t)(priv->data_addr));
	if(!temp) {
		temp = gmalloc(FS_BLOCK_SIZE);
		if(disk_driver->read(disk_driver, priv->data_addr, FS_SECTOR_PER_BLOCK, temp) <= 0) {
			printf("Reading root directory failed\n");
			gfree(temp);
			return -1;
		}
		if(!cache_set(driver->cache, (void*)(uintptr_t)(priv->data_addr), temp)) {
			printf("cache setting error\n");
			gfree(temp);
			return -2;
		}
	}

	// Return entry which corresponds to file name 
	BFSDir* bfs_dir = (BFSDir*)temp;
	int len = strlen(name);

	for(int i = 2; i < 0x40; i++) {
		if(strncmp((char*)bfs_dir[i].name, name, len) == 0) {
			BFSInode* inode = get_inode_entry(driver, bfs_dir[i].inode);
			
			if(inode == 0) {
				printf("Inode empty\n");
				return -3;
			}

			// Byte offset from super block - block offset from super block
			file->size = inode->eof - inode->first * FS_SECTOR_SIZE + 1;
			file->sector = priv->reserved_addr + inode->first;

			return bfs_dir[i].inode;
		}
	}

	return -4;
}

/* High level function */
static File* bfs_open(FileSystemDriver* driver, File* file, const char* file_name, char* flags) { 
	int inode = find_directory_entry(driver, file_name, file);

	if(inode < 0)  
		return NULL;

	int len = strlen(file_name);
	if(len > BFS_NAME_LEN - 1) 
		return NULL;

	memcpy(file->name, file_name, len + 1);
	file->type = FILE_TYPE_FILE;
	file->inode = inode;

	// Offset initialization
	file->offset = 0;
	//file->index = 0;

	return file;
}

static int bfs_close(FileSystemDriver* driver, File* file) { 
	// Nothing to do. Left for comfortability 
	return 0;
}

static int bfs_write(DiskDriver* driver, uint32_t sector, size_t sector_count, void* buffer) {
	if(driver->write(driver, sector, sector_count, buffer) < 0)
		return -FILE_ERR_IO;
	
	return 0;
}
static int bfs_read(DiskDriver* driver, uint32_t sector, size_t sector_count, void* buffer) {
	if(driver->read(driver, sector, sector_count, buffer) < 0)
		return -FILE_ERR_IO;

	return 0;
}

static int bfs_read_async(DiskDriver* driver, List* blocks, void(*callback)(List* blocks, int count, void* context), void* context) {
	driver->read_async(driver, blocks, FS_SECTOR_PER_BLOCK, callback, context);
	return 0;
}

static int bfs_write_async(DiskDriver* driver, List* blocks, void(*callback)(List* blocks, int count, void* context), void* context) {
	return driver->write_async(driver, blocks, FS_SECTOR_PER_BLOCK, callback, context);
}

static off_t bfs_lseek(FileSystemDriver* driver, File* file, off_t offset, int whence) {
	// BFS doesn't support having pointer over file size
	switch(whence) {
		// From start of file
		case FS_SEEK_SET :
			if(offset < 0) 
				offset = 0;
			else if(offset > file->size)
				offset = file->size;

			break;

		// From current offset of file
		case FS_SEEK_CUR :
			if(offset < 0 && file->offset + offset < 0) 
				offset = 0;
			else if(offset + file->offset > file->size)
				offset = file->size;
			
			break;

		// From end of file
		case FS_SEEK_END :
			if(offset > 0)
				offset = file->size;
			else if(-offset > file->size) 
				offset = 0;

			break;

		default : 
			return 0;
	}
	
	file->offset = offset;
	//file->index = offset / (FS_SECTOR_SIZE * FS_SECTOR_PER_BLOCK);

	return offset;
}

static File* bfs_opendir(FileSystemDriver* driver, File* dir, const char* dir_name) {

	DiskDriver* disk_driver = driver->driver;
	// BFS only have root directory, so forget about the name
	BFSInode* inode = get_inode_entry(driver, 2); // Root directory is at inode 2
	if(!inode)
		return NULL;

	// Byte offset from super block - block offset from super block * 512 = byte size
	dir->size = inode->eof - inode->first * FS_SECTOR_SIZE + 1;

	// Fill in directory buffer  
	int count = dir->size / sizeof(BFSDir); 
	dir->buf = malloc(sizeof(Dirent) * count);
	if(!dir->buf)
		return NULL;

	bzero(dir->buf, sizeof(Dirent) * count);

	Dirent* dirent = (Dirent*)dir->buf;
	
	BFSPriv* priv = driver->priv;

	// NOTE: Assume that root directory is smaller than one cluster
	void* temp = cache_get(driver->cache, (void*)(uintptr_t)(priv->data_addr));
	if(!temp) {
		temp = gmalloc(FS_BLOCK_SIZE);
		if(disk_driver->read(disk_driver, priv->data_addr, FS_SECTOR_PER_BLOCK, temp) <= 0) {
			printf("Reading root directory failed\n");
			gfree(temp);
			return NULL;
		}
		if(!cache_set(driver->cache, (void*)(uintptr_t)(priv->data_addr), temp)) {
			printf("cache setting error\n");
			gfree(temp);
			return NULL;
		}
	}

	BFSDir* bfs_dir = (BFSDir*)temp;

	for(int i = 0; i < count; i++) {
		dirent[i].inode = bfs_dir->inode; 
		memcpy(&dirent[i].name, bfs_dir->name, BFS_NAME_LEN); 
		bfs_dir++;
	}

	// Root directory name is "/"
	memcpy(dir->name, "/", 2);
	dir->type = FILE_TYPE_DIR;

	// Offset initialization
	//dir->index = 0;
	dir->offset = 0;

	return dir;
}

static Dirent* bfs_readdir(FileSystemDriver* driver, File* dir) {
	int index = dir->offset / sizeof(BFSDir);

	Dirent* dirent = (Dirent*)(dir->buf + index * sizeof(Dirent));

	while(dir->offset != dir->size) { 
		if(dirent->name != NULL) {
			dir->offset += sizeof(BFSDir);

			return dirent;
		}

		dir->offset += sizeof(BFSDir);
		dirent++;
	}	

	return NULL;
}

static void bfs_rewinddir(FileSystemDriver* driver, File* dir) {
	// Not implemented yet
}

static int bfs_closedir(FileSystemDriver* driver, File* dir) {
	if(dir->buf)
		free(dir->buf);
	else 
		return -1;

	return 0;
}

FileSystemDriver bfs_driver = {
	.type = FS_TYPE_BFS,

	.mount = bfs_mount,
	.umount = bfs_umount,

	.open = bfs_open,
	.close = bfs_close,
	.read = bfs_read,
	.write = bfs_write,
	.read_async = bfs_read_async,
	.write_async = bfs_write_async,
	.lseek = bfs_lseek,

	.opendir = bfs_opendir,
	.readdir = bfs_readdir,
	.closedir = bfs_closedir,
	.rewinddir = bfs_rewinddir,
};

bool bfs_init() {
	// File system driver register
	return fs_register(&bfs_driver);
}
