#include "string.h"
#include "fs.h"
#include "pata.h"
#include "disk.h"
#include "../cpu.h"
#include <malloc.h>
#include <util/cache.h>
#include <util/event.h>

static FileSystemDriver* drivers[DISK_MAX_DRIVERS];
static Map* mounts;	// Key: path string, Value: file system driver

bool fs_init() {
	// Mounting map initialization 
	mounts = map_create(FS_MAX_DRIVERS * FS_MOUNTING_POINTS, map_string_hash, map_string_equals, NULL);	
	if(!mounts)
		return false; // Memory allocation fail
	
	uint32_t ids[DISK_MAX_DRIVERS * DISK_AVAIL_DEVICES];
	int count;
	
	if((count = disk_ids(ids, DISK_MAX_DRIVERS * DISK_AVAIL_DEVICES)) < 0) {
		printf("Disk not found\n");
		return false;
	}

	return false;
}

int fs_mount(uint32_t disk, uint8_t partition, int type, const char* path) {
	if(map_contains(mounts, (void*)path)) {
		printf("path '%s' is already mounted\n", path);
		return -1;
	}

	// Find file system driver 
	FileSystemDriver* driver = NULL;
	for(int i = 0; i < DISK_MAX_DRIVERS; i++) {
		if(drivers[i] == NULL)
			break;
			
		if(drivers[i]->type == type) {
			driver = drivers[i];
			break;
		}
	}

	if(driver == NULL) {
		printf("Required file system not found\n");
		return -2; // Required file system not found
	}

	// Cache size is (FS_CACHE_BLOCK * FS_BLOCK_SIZE(normally 4K))
	Cache* cache = cache_create(FS_CACHE_BLOCK, free, NULL); 
	if(!cache) {
		printf("Create cache fail\n");
		return -3;
	}
	driver->cache = cache;

	DiskDriver* disk_driver = disk_get(disk);
	if(!disk_driver) {
		printf("Disk not found\n");
		return -4; // Disk not found
	}

	PartEntry* part_entry = &disk_driver->boot_sector->part_entry[partition];

	// Check if a boot sector ends with 0x55aa
	if(disk_driver->boot_sector->boot_signature[0] != 0x55 || disk_driver->boot_sector->boot_signature[1] != 0xaa) {
		disk_driver->type = DISK_TYPE_RAMDISK;
		part_entry->first_lba = 0;
	}

	if(driver->mount(driver, disk_driver, part_entry->first_lba, part_entry->num_of_sec) < 0) {
		printf("Bad superblock\n");
		return -5; // Bad superblock
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

	return map_get(mounts, (void*)path_prefix);
}

int fs_umount(const char* path) {
	FileSystemDriver* driver = fs_driver(path);

	if(!driver) 
		return -2; // Reuqired file system not found

	if(driver->umount(driver) < 0)
		return -3; // Memory free error

	map_remove(mounts, (void*)path);

	return 0;
}

bool fs_register(FileSystemDriver* driver) {
	for(int i = 0; i < DISK_MAX_DRIVERS; i++) {
		if(drivers[i] == NULL) {
			drivers[i] = driver;
			goto done;
		}
	}
	
	// Disk driver count exceed
	return false;

done:
	return true;
}

ssize_t fs_write(File* file, void* buffer, size_t size) {
	FileSystemDriver* driver = file->driver;
	if(!driver)
		return -FILE_ERR_FSDRIVER;

	return driver->write(driver, file->priv, buffer, size);
}

ssize_t fs_read(File* file, void* buffer, size_t size) {
	FileSystemDriver* driver = file->driver;
	if(!driver)
		return -FILE_ERR_FSDRIVER;

	return driver->read(driver, file->priv, buffer, size);
}

int fs_read_async(File* file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context) {
	FileSystemDriver* driver = file->driver;
	if(!driver)
		return -FILE_ERR_FSDRIVER;

	return driver->read_async(driver, file->priv, size, callback, context);
}

int fs_write_async(File* file, void* buffer, size_t size, void(*callback)(void* buffer, int len, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2) {
	FileSystemDriver* driver = file->driver;
	if(!driver)
		return -FILE_ERR_FSDRIVER;

	int ret = driver->write_async(driver, file->priv, buffer, size, callback, context, sync_callback, context2);
	
	return ret;
}

//typedef struct {
//	void(*callback)(void* buffer, size_t len, void* context);
//	void*		usr_buffer;
//	List*		blocks;
//	void*		context;
//	Cache*		cache;
//	void*		buffer[2];
//	size_t		offset[2];
//} FragContext;
//
//static void frag_read_callback(List* fragments, int count, void* context) {
//	FragContext* frag_context = context;
//	// If read error occurs
//	if(count < 0) {
//		list_destroy(frag_context->blocks);
//		list_destroy(fragments);
//		if(frag_context->callback)
//			frag_context->callback(frag_context->usr_buffer, count, frag_context->context);
//		free(context);
//		return;
//	}
//
//	for(int i = 0; i < count; i++) {
//		// Modify fragments to make a block
//		BufferBlock* block = list_remove_first(fragments);
//		memcpy(block->buffer + frag_context->offset[i], frag_context->buffer[i], block->size);
//
//		// Move modified block into the clean list
//		list_add(frag_context->blocks, block);
//	}
//
//	if(fragments)
//		list_destroy(fragments);
//
//	size_t len = 0;
//	Cache* cache = frag_context->cache;
//	ListIterator iter;
//	list_iterator_init(&iter, frag_context->blocks);
//	while(list_iterator_has_next(&iter)) {
//		BufferBlock* block = list_iterator_next(&iter);
//		list_iterator_remove(&iter);
//
//		// Move clean blocks into write buffers
//		list_add(write_buffers, block);
//
//		// Cache update
//		void* data = cache_get(cache, (void*)(uintptr_t)block->sector);
//		if(data)
//			memcpy(data, block->buffer, FS_BLOCK_SIZE);
//
//		len += block->size;
//	}
//
//	list_destroy(frag_context->blocks);
//
//	// Callback
//	frag_context->callback(frag_context->usr_buffer, len, frag_context->context);
//	free(frag_context);
//}
//
//static bool write_tick(void* context) {
//	frag_read_callback(NULL, 0, context);
//	return false;
//}
//
//typedef struct {
//	void(*callback)(List* blocks, int count, void* context);
//	void*			context;
//	FileSystemDriver*	driver;
//	uint64_t*		event_id;
//} SyncContext;
//
//// Flush the write buffer and sync with the disk
//static bool sync_tick(void* context) {
//	SyncContext* sync_context= context;
//	FileSystemDriver* driver = sync_context->driver;
//	if(!driver->write_async(driver->driver, write_buffers, sync_context->callback, sync_context->context)) {
//		*sync_context->event_id = 0;
//		free(sync_context);
//		return false;
//	}
//
//	return true;
//}
//
//typedef struct {
//	void(*callback)(void* buffer, size_t len, void* context);
//	void(*sync_callback)(int errno, void* context2);
//	void*		context;
//	void*		sync_context;
//	File*		file;
//	void*		buffer;
//	size_t		size;
//} LazyWriteContext;
//
//typedef struct {
//	void(*callback)(int errno, void* context);
//	void*		context;
//} SyncedContext;
//
//static void synced_callback(List* blocks, int len, void* context) {
//	// Call the sync callback
//	SyncedContext* synced_context = context;
//	if(synced_context->callback)
//		synced_context->callback(len, synced_context->context);
//	free(synced_context);
//
//	// If there are another write operations in the wating list. Do it now.
//	ListIterator iter;
//	list_iterator_init(&iter, wait_lists);
//	while(list_iterator_has_next(&iter)) {
//		LazyWriteContext* lazy_write = list_iterator_next(&iter);
//		list_iterator_remove(&iter);
//
//		// If write operation is done well, free the context and remove from the list
//		if(fs_write_async(lazy_write->file, lazy_write->buffer, lazy_write->size, lazy_write->callback, lazy_write->context, lazy_write->sync_callback, lazy_write->sync_context)) {
//			free(lazy_write);
//		} else {	// If there's no room in the write buffer
//			break;
//		}
//	}
//}
//
//bool fs_write_async(File* file, void* buffer, size_t size, void(*callback)(void* buffer, size_t len, void* context), void* context, void(*sync_callback)(int errno, void* context2), void* context2) {
//#ifdef _KERNEL_
//#define MIN(x, y) (((x) < (y)) ? (x) : (y))
//	// Check request user block count > file size
//	size = MIN(file->size - file->offset, size);
//	// Temporarily set the maximum size : FS_BLOCK_SIZE(4K) * 2560 : 10M
//	size = MIN(size, FS_BLOCK_SIZE * FS_WRITE_BUF_SIZE);
//
//	static uint64_t event_id = 0;
//	// If write buffer is full
//	if(FS_WRITE_BUF_SIZE - list_size(write_buffers) < size / FS_BLOCK_SIZE) {
//		LazyWriteContext* lazy_write = malloc(sizeof(LazyWriteContext));
//		lazy_write->callback = callback;
//		lazy_write->sync_callback = sync_callback;
//		lazy_write->context = context;
//		lazy_write->sync_context= context2;
//		lazy_write->file = file;
//		lazy_write->buffer = buffer;
//		lazy_write->size = size;
//
//		// Add context information into the list
//		list_add(wait_lists, lazy_write);
//
//		// Update the period of timer event to fire
//		event_timer_update(event_id, 0);
//
//		return false;
//	} else if(event_id != 0) {
//		event_timer_update(event_id, 100000);
//	} else {	// If there's no timer event registered yet
//		SyncedContext* synced_context = malloc(sizeof(SyncContext));
//		synced_context->callback = sync_callback;
//		synced_context->context = context2;
//
//		SyncContext* sync_context= malloc(sizeof(SyncContext));
//		sync_context->driver = file->driver;
//		sync_context->callback = synced_callback;
//		sync_context->context = synced_context;
//		sync_context->event_id = &event_id;
//
//		event_id = event_timer_add(sync_tick, sync_context, 100000, 100000);
//	}
//
//	int len = 0;
//	int head = 0;
//	size_t offset = file->offset;
//	FragContext* frag_context = malloc(sizeof(FragContext));
//	List* blocks = list_create(NULL);
//	List* fragments = NULL;
//
//	while(len < size) {
//		BufferBlock* block = malloc(sizeof(BufferBlock));
//		if(!block)
//			break;
//
//		block->sector = file->sector + (offset / FS_BLOCK_SIZE * FS_SECTOR_PER_BLOCK);
//		block->size = MIN(FS_BLOCK_SIZE - (offset % FS_BLOCK_SIZE), size - len);
//		block->buffer = buffer + len;
//
//		// When it writes less then FS_BLOCK_SIZE we should modify the block.
//		if(block->size != FS_BLOCK_SIZE) {
//			frag_context->offset[head] = offset % FS_BLOCK_SIZE;
//			frag_context->buffer[head++] = block->buffer;
//			block->buffer = NULL;
//
//			if(!fragments)
//				fragments = list_create(NULL);
//
//			// Fragments list will be sent to driver to read
//			list_add(fragments, block);
//		} else {
//			list_add(blocks, block);
//		}
//
//		len += block->size;
//		offset += block->size;
//	}
//
//	frag_context->callback = callback;
//	frag_context->context = context;
//	frag_context->usr_buffer = buffer;
//	frag_context->blocks = blocks;
//	frag_context->cache = driver->cache;
//
//	if(fragments) {
//		FileSystemDriver* driver = file->driver;
//		driver->read_async(driver->driver, fragments, frag_read_callback, frag_context);
//	} else {
//		// Need to add event to prevent an infinite cycle
//		event_busy_add(write_tick, frag_context);
//	}
//
//	return true;
//#endif 
//}
