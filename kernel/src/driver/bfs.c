#include <malloc.h>
#include <util/event.h>
#include "bfs.h"
#include "string.h"
#include "../malloc.h"
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

	List*		read_buffers;
	List*		write_buffers;
	List*		wait_lists;
} BFSPriv;


/** 
 * PacketNgin doesn't have partition table in MBR. So we have to find file system first.
 */

// TODO: Checking superblock only, not investigate whole disks
static int bfs_mount(FileSystemDriver* fs_driver, DiskDriver* disk_driver, uint32_t lba) {
	// Size of loader is now about 60kB. Investigate til 200kB (50 Clusters)
	BFSSuperBlock* sb;
	void* temp = malloc(FS_BLOCK_SIZE);
	if(disk_driver->read(disk_driver, lba, 8, temp) > 0) {
		sb = (BFSSuperBlock*)temp;
		if(sb->magic == BFS_MAGIC) {
			// Read 1 more sector for 8th sector
			if(disk_driver->read(disk_driver, lba + 1, 1, temp) > 0) {
				// Create private data structure
				fs_driver->priv = malloc(sizeof(BFSPriv));
				BFSPriv* priv = fs_driver->priv;

				priv->reserved_addr = lba;
				priv->reserved_size = 1;

				priv->link_table_addr = lba + 1;
				BFSInode* inode = (BFSInode*)temp;
				priv->link_table_size = inode->first /* Start of Data */ 
					- 1 /* Super block */;

				priv->data_addr = lba + inode->first;

				int size = sb->end - sb->start; // Total size (bytes)
				priv->total_cluster_count = (size - 4095) / 4096;

				priv->read_buffers = list_create(NULL);
				priv->write_buffers = list_create(NULL);
				priv->wait_lists = list_create(NULL);

				// Disk driver attachment
				fs_driver->driver = disk_driver;

				return 0;
			}
		}

		sb = (BFSSuperBlock*)((uintptr_t)sb + 512);
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
		temp = malloc(FS_BLOCK_SIZE);
		if(disk_driver->read(disk_driver, priv->link_table_addr, FS_SECTOR_PER_BLOCK, temp) <= 0) {
			printf("Reading root directory failed\n");
			free(temp);
			return NULL;
		}
		if(!cache_set(driver->cache, (void*)(uintptr_t)(priv->link_table_addr), temp)) {
			printf("cache setting error\n");
			free(temp);
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
static int find_directory_entry(FileSystemDriver* driver, const char* name, BFSFile* file) { 
	BFSPriv* priv = driver->priv;
	DiskDriver* disk_driver = driver->driver;

	// Read root directory
	// NOTE: Assume that root directory is smaller than one cluster
	void* temp = cache_get(driver->cache, (void*)(uintptr_t)(priv->data_addr));
	if(!temp) {
		temp = malloc(FS_BLOCK_SIZE);
		if(disk_driver->read(disk_driver, priv->data_addr, FS_SECTOR_PER_BLOCK, temp) <= 0) {
			printf("Reading root directory failed\n");
			free(temp);
			return -1;
		}
		if(!cache_set(driver->cache, (void*)(uintptr_t)(priv->data_addr), temp)) {
			printf("cache setting error\n");
			free(temp);
			return -2;
		}
	}

	// Return entry which corresponds to file name 
	BFSDir* bfs_dir = (BFSDir*)temp;

	for(int i = 2; i < 0x40; i++) {
		if(strcmp((char*)bfs_dir[i].name, name) == 0) {
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
static int bfs_open(FileSystemDriver* driver, const char* file_name, char* flags, void** priv) {
	BFSFile* file = malloc(sizeof(BFSFile));
	if(!file)
		return -1;

	int inode = find_directory_entry(driver, file_name, file);

	if(inode < 0)  
		return -2;

	int len = strlen(file_name);
	if(len > BFS_NAME_LEN - 1) 
		return -3;

	memcpy(file->name, file_name, len + 1);
	file->inode = inode;

	// Offset initialization
	file->offset = 0;
	//file->index = 0;

	*priv = file;
	return FILE_OK;
}

static int bfs_close(FileSystemDriver* driver, void* file) { 
	if(file)
		free(file);

	return 0;
}

static int bfs_write(FileSystemDriver* driver, void* _file, void* buffer, size_t size) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
	BFSFile* file = (BFSFile*)_file;
	if((size_t)file->offset >= file->size)
		return FILE_EOF;

	DiskDriver* disk_driver = driver->driver;
	size_t write_count = 0;
	size_t total_size;

	total_size =  MIN(file->size - file->offset, size);

	while(write_count != total_size) {
		uint32_t offset_in_cluster = file->offset % FS_BLOCK_SIZE;
		uint32_t sector = file->sector + (file->offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		size_t write_size = MIN(FS_BLOCK_SIZE - offset_in_cluster, total_size - write_count);
		void* write_buf = buffer + write_count;

		void* data = cache_get(driver->cache, (void*)(uintptr_t)sector);
		if(data) 
			memcpy(data, write_buf, FS_BLOCK_SIZE);

		// Write operation
		if(disk_driver->write(disk_driver, sector, FS_SECTOR_PER_BLOCK, write_buf)) {
			printf("Write fail\n");
			return -2;
		}

		write_count += write_size;
		file->offset += write_size;
	}

	return write_count;
}

static int bfs_read(FileSystemDriver* driver, void* _file, void* buffer, size_t size) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
	BFSFile* file = (BFSFile*)_file;
	if((size_t)file->offset >= file->size)
		return FILE_EOF;

	DiskDriver* disk_driver = driver->driver;
	size_t read_count = 0;
	size_t total_size;

	total_size =  MIN(file->size - file->offset, size);

	while(read_count != total_size) {
		uint32_t offset_in_cluster = file->offset % FS_BLOCK_SIZE;
		uint32_t sector = file->sector + (file->offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		size_t read_size = MIN(FS_BLOCK_SIZE - offset_in_cluster, total_size - read_count);

		void* read_buf = cache_get(driver->cache, (void*)(uintptr_t)sector);

		if(!read_buf) {
			read_buf = malloc(FS_BLOCK_SIZE);
			if(!read_buf) {
				printf("malloc error\n");
				return -1;
			}

			if(disk_driver->read(disk_driver, sector, FS_SECTOR_PER_BLOCK, read_buf) <= 0) {
				printf("read error\n");
				free(read_buf);
				return -2;
			}

			if(!cache_set(driver->cache, (void*)(uintptr_t)sector, read_buf)) {
				printf("cache setting error\n");
				free(read_buf);
				return -3;
			}
		}

		memcpy((void*)((uint8_t*)buffer + read_count), (void*)(read_buf + offset_in_cluster), read_size);

		read_count += read_size;
		file->offset += read_size;
	}

	return read_count;
}

typedef struct {
	bool(*callback)(List* blocks, int success, void* context);
	void*		context;
	uint32_t	offset;
	BFSFile*	file;
	List*		read_buffers;
	Cache*		cache;
} ReadContext;

/* 
 * Set cache datas which are read successfully from the disk
 * 
 * @param blocks a list containing blocks
 * @param count a number of successful read
 * @param context a context
 */
static void read_callback(List* blocks, int count, void* context) {
	ReadContext* read_context = context;
	BFSFile* file = read_context->file;
	Cache* cache = read_context->cache;

	int read_count = 0;
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		BufferBlock* block = list_iterator_next(&iter);

		if(read_count++ < count) {
			// Cache setting
			cache_set(cache, (void*)(uintptr_t)block->sector, block->buffer);
			file->offset += block->size;
		} else {
			list_iterator_remove(&iter);
			free(block);
		}
	}

	BufferBlock* first_block = list_get_first(blocks);
	if(first_block)
		first_block->buffer = (uint8_t*)first_block->buffer - read_context->offset;

	read_context->callback(blocks, count, read_context->context);
	free(read_context);
}

typedef struct {
	int count;
	void* context;
	List* read_buffers;
} ReadTickContext;

static bool read_tick(void* context) {
	ReadTickContext* read_tick_ctx = context;
	List* read_buffers = read_tick_ctx->read_buffers;
	read_callback(read_buffers, read_tick_ctx->count, read_tick_ctx->context);
	free(read_tick_ctx);
	
	return false;
}

static int bfs_read_async(FileSystemDriver* driver, void* _file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
	BFSFile* file = (BFSFile*)_file;
	if((size_t)file->offset >= file->size)
		return FILE_EOF;

	BFSPriv* priv = (BFSPriv*)driver->priv;
	List* read_buffers = priv->read_buffers;
	DiskDriver* disk_driver = driver->driver;

	// If there is no space in the buffer
	if(FS_NUMBER_OF_BLOCKS - list_size(read_buffers) <= 0)
		return -FILE_ERR_NOBUFS;

	// Optimize the size that we can handle
	size = MIN(file->size - file->offset, size);
	size = MIN(FS_CACHE_BLOCK * FS_BLOCK_SIZE, size);

	ReadContext* read_context = malloc(sizeof(ReadContext));
	read_context->offset = file->offset % FS_BLOCK_SIZE; // offset for first block
	read_context->callback = callback;
	read_context->context = context;
	read_context->cache = driver->cache;
	read_context->file = file;

	size_t total_size = size;
	size_t offset = file->offset;
	size_t len = 0;
	int count = 0;
	bool none_cached = false;

	while(len < total_size) {
		BufferBlock* block = malloc(sizeof(BufferBlock));
		if(!block) 
			break;

		block->sector = file->sector + (offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		block->size = MIN(FS_BLOCK_SIZE - (offset % FS_BLOCK_SIZE), total_size - len);

		// Check if cache has a specific buffer
		block->buffer = cache_get(driver->cache, (void*)(uintptr_t)block->sector);
		if(!block->buffer)
			none_cached = true;

		// Add blocks into the list to copy into the buffer from callback
		list_add(read_buffers, block);

		len += block->size;
		offset += block->size;
		count++;
	}

	// If it needs to read from the disk
	if(none_cached) {
		disk_driver->read_async(disk_driver, read_buffers, FS_SECTOR_PER_BLOCK, read_callback, read_context);
#ifdef _KERNEL_
	} else {	// If all the blocks are from the cache
		ReadTickContext* read_tick_ctx = malloc(sizeof(ReadTickContext));
		read_tick_ctx->count = count;
		read_tick_ctx->context = read_context;
		read_tick_ctx->read_buffers = read_buffers;

		// Need to add event to prevent an infinite cycle
		event_busy_add(read_tick, read_tick_ctx);
#endif 
	}

	return 1;
}

//static int bfs_write_async(DiskDriver* driver, List* blocks, void(*callback)(List* blocks, int count, void* context), void* context) {
//	return driver->write_async(driver, blocks, FS_SECTOR_PER_BLOCK, callback, context);
//}

static off_t bfs_lseek(FileSystemDriver* driver, void* _file, off_t offset, int whence) {
	BFSFile* file = (BFSFile*)_file;
	// BFS doesn't support having pointer over file size
	switch(whence) {
		// From start of file
		case FS_SEEK_SET :
			if(offset < 0) 
				offset = 0;
			else if((size_t)offset > file->size)
				offset = file->size;

			break;

		// From current offset of file
		case FS_SEEK_CUR :
			if(offset < 0 && file->offset + offset < 0) 
				offset = 0;
			else if((size_t)(offset + file->offset) > file->size)
				offset = file->size;
			
			break;

		// From end of file
		case FS_SEEK_END :
			if(offset > 0)
				offset = file->size;
			else if((size_t)-offset > file->size) 
				offset = 0;

			break;

		default : 
			return 0;
	}
	
	file->offset = offset;
	//file->index = offset / (FS_SECTOR_SIZE * FS_SECTOR_PER_BLOCK);

	return offset;
}

static void* bfs_opendir(FileSystemDriver* driver, const char* dir_name) {
	BFSFile* dir = malloc(sizeof(BFSFile));
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
		temp = malloc(FS_BLOCK_SIZE);
		if(disk_driver->read(disk_driver, priv->data_addr, FS_SECTOR_PER_BLOCK, temp) <= 0) {
			printf("Reading root directory failed\n");
			free(temp);
			return NULL;
		}
		if(!cache_set(driver->cache, (void*)(uintptr_t)(priv->data_addr), temp)) {
			printf("cache setting error\n");
			free(temp);
			return NULL;
		}
	}

	BFSDir* bfs_dir = (BFSDir*)temp;

	for(int i = 0; i < count; i++) {
		//dirent[i].inode = bfs_dir->inode; 
		memcpy(&dirent[i].name, bfs_dir->name, BFS_NAME_LEN); 
		bfs_dir++;
	}

	// Root directory name is "/"
	memcpy(dir->name, "/", 2);

	// Offset initialization
	//dir->index = 0;
	dir->offset = 0;

	return dir;
}

static int bfs_readdir(FileSystemDriver* driver, void* _dir, Dirent* dirent) {
	BFSFile* dir = (BFSFile*)_dir;
	if((size_t)dir->offset == dir->size) 
		return FILE_EOF;

	int index = dir->offset / sizeof(BFSDir);

	Dirent* kdirent = (Dirent*)(dir->buf + index * sizeof(Dirent));

	while((size_t)dir->offset != dir->size) { 
		if(kdirent->name != NULL) {
			dir->offset += sizeof(BFSDir);
			strcpy(dirent->name, kdirent->name);

			return 1;
		}

		dir->offset += sizeof(BFSDir);
		kdirent++;
	}	

	return -2;
}

static void bfs_rewinddir(FileSystemDriver* driver, void* _dir) {
	// Not implemented yet
}

static int bfs_closedir(FileSystemDriver* driver, void* _dir) {
	BFSFile* dir = (BFSFile*)_dir;
	if(dir->buf)
		free(dir->buf);
	else 
		return -1;

	return 0;
}

static int bfs_file_size(void* _file) {
	BFSFile* file = (BFSFile*)_file;
	return file->size;
}

static bool bfs_check_sync(void* _file) {
	return true;
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
//	.write_async = bfs_write_async,
	.lseek = bfs_lseek,
	.file_size = bfs_file_size,
	.check_sync = bfs_check_sync,

	.opendir = bfs_opendir,
	.readdir = bfs_readdir,
	.closedir = bfs_closedir,
	.rewinddir = bfs_rewinddir,
};

bool bfs_init() {
	// File system driver register
	return fs_register(&bfs_driver);
}
