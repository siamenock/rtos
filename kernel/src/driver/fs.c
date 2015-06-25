#include "string.h"
#include "fs.h"
#include "pata.h"
#include "disk.h"
#include <malloc.h>
#include <util/cache.h>

static List* drivers;
static Map* mounts;	// Key: path string, Value: file system driver

bool fs_init() {
	// Mounting map initialization 
	mounts = map_create((FS_MAX_DRIVERS * FS_MOUNTING_POINTS), map_string_hash, map_string_equals, NULL);	

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
				// We retain 10 clusters cache (40kB)
				driver->cache = cache_create(10, free, NULL); 
				if(!driver->cache) {
					printf("Create cache fail\n");
					return false;
				}

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

ssize_t fs_read(FileSystemDriver* driver, uint32_t lba, size_t sector_count, void* buf) {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

	DiskDriver* disk_driver = driver->driver;

	// Start/end LBA of cluster to read
	int start = (lba / FS_SECTOR_PER_CLUSTER) * FS_SECTOR_PER_CLUSTER;
	int end = ((lba + sector_count) / FS_SECTOR_PER_CLUSTER) * FS_SECTOR_PER_CLUSTER;

	uint8_t* ptr = buf;
	int total_count = sector_count * FS_SECTOR_SIZE;
	for(int i = start; i <= end; i += FS_SECTOR_PER_CLUSTER) {
		// Check cache 
		void* data = cache_get(driver->cache, (void*)(uintptr_t)i);
		if(!data) {
			data = malloc(FS_CLUSTER_SIZE);
			if(!data) {
				printf("malloc error\n");
				return -1;
			} 

			if(disk_driver->read(disk_driver, i, FS_SECTOR_PER_CLUSTER, data) < 0) {
				printf("disk read error\n");
				return -2;
			}

			if(!cache_set(driver->cache, (void*)(uintptr_t)i, data)) {
				printf("cache setting error\n");
				return -3;
			}
		}

		int offset = (lba > i) ? ((lba - start) * FS_SECTOR_SIZE) : 0;
		int size = MIN(total_count, FS_CLUSTER_SIZE - offset);
		memcpy(ptr, (void*)((uintptr_t)data + offset), size);

		total_count -= size;
		if(total_count > 0)
			ptr = (uint8_t*)((uintptr_t)ptr + size);
	}

	return sector_count * FS_SECTOR_SIZE;
}

ssize_t fs_write(FileSystemDriver* driver, uint32_t lba, size_t cluster_count, void* buf) {
	// Not yet implemented
	return 0;
}




