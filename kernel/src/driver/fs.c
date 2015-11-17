#include "string.h"
#include "fs.h"
#include "pata.h"
#include "disk.h"
#include "../cpu.h"
#include <malloc.h>
#include <gmalloc.h>
#include <util/cache.h>
#include <util/event.h>

static List* drivers;
static List* read_buffers;
static List* write_buffers;
static List* wait_lists;
static Cache* cache;
static Map* mounts;	// Key: path string, Value: file system driver

bool fs_init() {
	// Mounting map initialization 
	mounts = map_create(FS_MAX_DRIVERS * FS_MOUNTING_POINTS, map_string_hash, map_string_equals, NULL);	

	// There is no file system at all
	if(list_size(drivers) == 0) {
		printf("There is no file system at all\n");
		return false;
	}

	// Get disk IDs
	uint32_t ids[DISK_MAX_DRIVERS * DISK_AVAIL_DEVICES];
	int count;
	
	if((count = disk_ids(ids, DISK_MAX_DRIVERS * DISK_AVAIL_DEVICES)) < 0) {
		printf("Disk not found\n");
		return false;
	}

	// Try to mount root directory
	ListIterator iter;
	list_iterator_init(&iter, drivers);
	FileSystemDriver* driver;

	while(list_iterator_has_next(&iter)) {
		driver = list_iterator_next(&iter);

		for(int i = 0; i < count; i++) {
			if(fs_mount(driver->type, ids[i], "/") == 0) {
				// Cache size is (FS_CACHE_BLOCK * FS_BLOCK_SIZE(normally 4K))
				cache = cache_create(FS_CACHE_BLOCK, gfree, NULL); 
				if(!cache) {
					printf("Create cache fail\n");
					return false;
				}

				driver->cache = cache;
#ifdef _KERNEL_
				write_buffers = list_create(NULL);
				read_buffers = list_create(NULL);
				wait_lists = list_create(NULL);
#endif /* _KERNEL_ */

				return true;
			}
		}
	}

	printf("File system init fail\n");
	return false;
}

int fs_mount(int type, uint32_t device, const char* path) {
	if(list_size(drivers) == 0) 
		return -1; // File system not found 

	// Find file system driver 
	ListIterator iter;
	list_iterator_init(&iter, drivers);
	FileSystemDriver* driver;
	
	while(list_iterator_has_next(&iter)) {
		driver = list_iterator_next(&iter);
	
		if(driver->type == type) 
			break;
	}

	if(driver->type != type) {
		printf("Required file system not found\n");
		return -2; // Required file system not found
	}

	DiskDriver* disk_driver = disk_get(device);
	if(!disk_driver) {
		printf("Disk not found\n");
		return -3; // Disk not found
	}

	if(driver->mount(driver, disk_driver) < 0) {
		printf("Bad superblock\n");
		return -4; // Bad superblock
	}

	// Success - mounting information is filled from now
	map_put(mounts, (void*)path, driver);

	return 0;
}

FileSystemDriver* fs_driver(const char* path) {
	int len = 0;
	char* path_prefix = "";
	MapIterator iter;
	map_iterator_init(&iter, mounts);

	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		if(strstr(path, entry->key) == path) {
			int len2 = strlen(entry->key);
			if(len2 > len) {
				path_prefix = entry->key;
				len = len2;
			}
		}

	}

	return map_get(mounts, path_prefix);
}

int fs_umount(const char* path) {
	if(list_size(drivers) == 0)
		return -1; // File system not found 

	FileSystemDriver* driver = fs_driver(path);

	if(!driver) 
		return -2; // Reuqired file system not found

	if(driver->umount(driver) < 0)
		return -3; // Memory free error

	map_remove(mounts, (void*)path);

	return 0;
}

bool fs_register(const FileSystemDriver* driver) {
	// Driver list initialization
	if(!drivers) {
		drivers = list_create(NULL);
		if(!drivers)
			return false; // Memory allocation fail
	}

	if(list_size(drivers) == DISK_MAX_DRIVERS) 
		return false; // Disk driver table is full

	list_add(drivers, (void*)driver);

	return true;
}

ssize_t fs_write(File* file, void* buffer, size_t size) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

	FileSystemDriver* driver = file->driver;
	size_t write_count = 0;
	size_t total_size;

	total_size =  MIN(file->size - file->offset, size);

	while(write_count != total_size) {
		uint32_t offset_in_cluster = file->offset % FS_BLOCK_SIZE;
		uint32_t sector = file->sector + (file->offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		size_t write_size = MIN(FS_BLOCK_SIZE - offset_in_cluster,
				total_size - write_count);
		void* write_buf = buffer + write_count;

		void* data = cache_get(cache, (void*)(uintptr_t)sector);
		if(data) 
			memcpy(data, write_buf, FS_BLOCK_SIZE);

		// Write operation
		if(driver->write(driver->driver, sector, FS_SECTOR_PER_BLOCK, write_buf)) {
			printf("Read fail\n");
			return -2;
		}

		write_count += write_size;
		file->offset += write_size;
	}

	return write_count;
}

ssize_t fs_read(File* file, void* buffer, size_t size) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

	FileSystemDriver* driver = file->driver;
	size_t read_count = 0;
	size_t total_size;

	total_size =  MIN(file->size - file->offset, size);

	while(read_count != total_size) {
		uint32_t offset_in_cluster = file->offset % FS_BLOCK_SIZE;
		uint32_t sector = file->sector + (file->offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		size_t read_size = MIN(FS_BLOCK_SIZE - offset_in_cluster, total_size - read_count);

		void* read_buf = cache_get(cache, (void*)(uintptr_t)sector);

		if(!read_buf) {
			read_buf = gmalloc(FS_BLOCK_SIZE);
			if(!read_buf) {
				printf("malloc error\n");
				return -1;
			}

			if(driver->read(driver->driver, sector, FS_SECTOR_PER_BLOCK, read_buf) < 0) {
				printf("read error\n");
				gfree(read_buf);
				return -2;
			}

			if(!cache_set(cache, (void*)(uintptr_t)sector, read_buf)) {
				printf("cache setting error\n");
				gfree(read_buf);
				return -3;
			}
		}

		memcpy((uint32_t*)((uint8_t*)buffer + read_count),
				(uint32_t*)(read_buf + offset_in_cluster), read_size);

		read_count += read_size;
		file->offset += read_size;
	}

	return read_count;
}

int fs_read_async(File* file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
	// If there is no space in the buffer
	if(FS_NUMBER_OF_BLOCKS - list_size(read_buffers) == 0) {
		return -FILE_ERR_NOBUFS;
	}

	// Check request user block count > file size
	size = MIN(file->size - file->offset, size);
	// check request user block count > max cache blocks
	size = MIN(FS_CACHE_BLOCK * FS_BLOCK_SIZE, size);

	typedef struct {
		bool(*callback)(List* blocks, int count, void* context);
		void*		context;
		uint32_t	offset;
		File*		file;
	} ReadContext;

	FileSystemDriver* driver = file->driver;
	ReadContext* read_context= malloc(sizeof(ReadContext));
	read_context->offset = file->offset % FS_BLOCK_SIZE;
	read_context->callback= callback;
	read_context->context = context;
	read_context->file = file;

	/* 
	 * Set cache datas which are read successfully from the disk
	 * 
	 * @param blocks a list containing blocks
	 * @param count a number of successful read
	 * @param context a context
	 */
	void read_callback(List* blocks, int count, void* context) {
		ReadContext* read_context= context;
		File* file = read_context->file;

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

	size_t total_size = size;
	size_t offset = file->offset;
	size_t len = 0;
	int count = 0;
	bool none_cached = false;

	while(len < total_size) {
		BufferBlock* block= malloc(sizeof(BufferBlock));
		if(!block) 
			break;

		block->sector = file->sector + (offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		block->size = MIN(FS_BLOCK_SIZE - (offset % FS_BLOCK_SIZE), total_size - len);

		// Check if cache has a specific buffer
		block->buffer = cache_get(cache, (void*)(uintptr_t)block->sector);
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
		driver->read_async(driver->driver, read_buffers, read_callback, read_context);
#ifdef _KERNEL_
	} else {	// If all the blocks are from the cache
		typedef struct {
			int count;
			void* context;
		} ReadTickContext;

		bool read_tick(void* context) {
			ReadTickContext* read_tick_ctxt = context;
			read_callback(read_buffers, read_tick_ctxt->count, read_tick_ctxt->context);
			free(read_tick_ctxt);
			return false;
		}

		ReadTickContext* read_tick_ctxt = malloc(sizeof(ReadTickContext));
		read_tick_ctxt->count = count;
		read_tick_ctxt->context = read_context;

		// Need to add event to prevent an infinite cycle
		event_busy_add(read_tick, read_tick_ctxt);
#endif /* _KERNEL_ */ 
	}
	return 1;
}

typedef struct {
	void(*callback)(List* blocks, int count, void* context);
	void*			context;
	FileSystemDriver*	driver;
	uint64_t*		event_id;
} SyncContext;

// Flush the write buffer and sync with the disk
static bool sync_tick(void* context) {
	SyncContext* sync_context= context;
	FileSystemDriver* driver = sync_context->driver;
	if(!driver->write_async(driver->driver, write_buffers, sync_context->callback, sync_context->context)) {
		*sync_context->event_id = 0;
		free(sync_context);
		return false;
	}

	return true;
}

// TODO: Combine write and fragment
bool fs_write_async(File* file, void* buffer, size_t size, void(*callback)(void* buffer, size_t len, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2) {
#ifdef _KERNEL_
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
	// Check request user block count > file size
	size = MIN(file->size - file->offset, size);
	// Temporarily set the maximum size : FS_BLOCK_SIZE(4K) * 2560 : 10M
	size = MIN(size, FS_BLOCK_SIZE * FS_WRITE_BUF_SIZE);

	typedef struct {
		void(*callback)(void* buffer, size_t len, void* context);
		void(*sync_callback)(int errno, void* context2);
		void*		context;
		void*		sync_context;
		File*		file;
		void*		buffer;
		size_t		size;
	} LazyWriteContext;

	typedef struct {
		void(*callback)(int errno, void* context);
		void*		context;
	} SyncedContext;

	void synced_callback(List* blocks, int len, void* context) {
		// Call the sync callback
		SyncedContext* synced_context = context;
		if(synced_context->callback)
			synced_context->callback(len, synced_context->context);
		free(synced_context);

		// If there are another write operations in the wating list. Do it now.
		ListIterator iter;
		list_iterator_init(&iter, wait_lists);
		while(list_iterator_has_next(&iter)) {
			LazyWriteContext* lazy_write = list_iterator_next(&iter);
			list_iterator_remove(&iter);

			// If write operation is done well, free the context and remove from the list
			if(fs_write_async(lazy_write->file, lazy_write->buffer, lazy_write->size, lazy_write->callback, lazy_write->context, lazy_write->sync_callback, lazy_write->sync_context)) {
				free(lazy_write);
			} else {	// If there's no room in the write buffer
				break;
			}
		}
	}

	static uint64_t event_id = 0;
	// If write buffer is full
	if(FS_WRITE_BUF_SIZE - list_size(write_buffers) < size / FS_BLOCK_SIZE) {
		LazyWriteContext* lazy_write = malloc(sizeof(LazyWriteContext));
		lazy_write->callback = callback;
		lazy_write->sync_callback = sync_callback;
		lazy_write->context = context;
		lazy_write->sync_context= context2;
		lazy_write->file = file;
		lazy_write->buffer = buffer;
		lazy_write->size = size;

		// Add context information into the list
		list_add(wait_lists, lazy_write);

		// Update the period of timer event to fire
		event_timer_update(event_id, 0);

		return false;
	} else if(event_id) {
		event_timer_update(event_id, 100000);
	} else {	// If there's no timer event registered yet
		SyncedContext* synced_context = malloc(sizeof(SyncContext));
		synced_context->callback = sync_callback;
		synced_context->context = context2;

		SyncContext* sync_context= malloc(sizeof(SyncContext));
		sync_context->driver = file->driver;
		sync_context->callback = synced_callback;
		sync_context->context = synced_context;
		sync_context->event_id = &event_id;

		event_id = event_timer_add(sync_tick, sync_context, 100000, 100000);
	}

	typedef struct {
		void(*callback)(void* buffer, size_t len, void* context);
		void*		usr_buffer;
		List*		blocks;
		void*		context;
		void*		buffer[2];
		size_t		offset[2];
	} FragContext;

	int len = 0;
	int head = 0;
	size_t offset = file->offset;
	FragContext* frag_context = malloc(sizeof(FragContext));
	List* blocks = list_create(NULL);
	List* fragments = NULL;

	while(len < size) {
		BufferBlock* block = malloc(sizeof(BufferBlock));
		if(!block)
			break;

		block->sector = file->sector + (offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
		block->size = MIN(FS_BLOCK_SIZE - (offset % FS_BLOCK_SIZE), size - len);
		block->buffer = buffer + len;

		// When it writes less then FS_BLOCK_SIZE we should modify the block.
		if(block->size != FS_BLOCK_SIZE) {
			frag_context->offset[head] = offset % FS_BLOCK_SIZE;
			frag_context->buffer[head++] = block->buffer;
			block->buffer = NULL;

			if(!fragments)
				fragments = list_create(NULL);

			// Fragments list will be sent to driver to read
			list_add(fragments, block);
		} else {
			list_add(blocks, block);
		}

		len += block->size;
		offset += block->size;
	}

	void frag_read_callback(List* fragments, int count, void* context) {
		FragContext* frag_context = context;
		// If read error occurs
		if(count < 0) {
			list_destroy(frag_context->blocks);
			list_destroy(fragments);
			if(frag_context->callback)
				frag_context->callback(frag_context->usr_buffer, count, frag_context->context);
			free(context);
			return;
		}

		for(int i = 0; i < count; i++) {
			// Modify fragments to make a block
			BufferBlock* block = list_remove_first(fragments);
			memcpy(block->buffer + frag_context->offset[i], frag_context->buffer[i], block->size);

			// Move modified block into the clean list
			list_add(frag_context->blocks, block);
		}

		if(fragments)
			list_destroy(fragments);

		size_t len = 0;
		ListIterator iter;
		list_iterator_init(&iter, frag_context->blocks);
		while(list_iterator_has_next(&iter)) {
			BufferBlock* block = list_iterator_next(&iter);
			list_iterator_remove(&iter);

			// Move clean blocks into write buffers
			list_add(write_buffers, block);

			// Cache update
			void* data = cache_get(cache, (void*)(uintptr_t)block->sector);
			if(data)
				memcpy(data, block->buffer, FS_BLOCK_SIZE);

			len += block->size;
		}

		list_destroy(frag_context->blocks);

		// Callback
		frag_context->callback(frag_context->usr_buffer, len, frag_context->context);
		free(frag_context);
	}

	frag_context->callback = callback;
	frag_context->context = context;
	frag_context->usr_buffer = buffer;
	frag_context->blocks = blocks;

	if(fragments) {
		FileSystemDriver* driver = file->driver;
		driver->read_async(driver->driver, fragments, frag_read_callback, frag_context);
	} else {
		bool write_tick(void* context) {
			frag_read_callback(NULL, 0, context);
			return false;
		}

		// Need to add event to prevent an infinite cycle
		event_busy_add(write_tick, frag_context);
	}

	return true;
#endif /* _KERNEL_ */ 
}
