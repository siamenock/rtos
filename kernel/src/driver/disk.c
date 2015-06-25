#include <stddef.h>
#include "disk.h"
#include "util/list.h"
#include "util/map.h"

static List* drivers;
static Map* disks;

bool disk_init0() {
	drivers = list_create(NULL);
	
	return drivers != NULL;
}

bool disk_register(const DiskDriver* driver) {
	if(list_size(drivers) >= DISK_MAX_DRIVERS) 
		return false; // Disk driver table is full

	list_add(drivers, (void*)driver);

	return true;
}

bool disk_init() {
	// Disk map initialization 
	disks = map_create(DISK_MAX_DRIVERS * DISK_AVAIL_DEVICES, map_uint64_hash, map_uint64_equals, NULL);	

	// There is no disks at all
	if(list_size(drivers) == 0)
		return false;

	// Disk drivers initialization
	ListIterator iter;
	list_iterator_init(&iter, drivers);
	DiskDriver* driver;

	while(list_iterator_has_next(&iter)) {
		driver = list_iterator_next(&iter);

		DiskDriver* temp[DISK_AVAIL_DEVICES]; 
		int count = driver->init(driver, temp);
		if(count < 0) {
			// Initialization fail
			continue;
		}
		
		for(int i = 0; i < count; i++) {
			// Key is 32bit type + number 
			uint32_t key = (temp[i]->type << 16) | (temp[i]->number);
			if(map_put(disks, (void*)(uintptr_t)key, temp[i]) == false) 
				return false;
		}
	}

	return true;
}

size_t disk_count() {
	return map_size(disks);
}

int disk_ids(uint32_t* ids, int size) {
	size_t count = disk_count();

	if(count == 0)
		return -1; // No disks

	if(size < count) 
		return -2; // ID buffer too small

	MapIterator iter;
	map_iterator_init(&iter, disks);
	int i = 0;
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		ids[i++] = (uint32_t)(uintptr_t)entry->key;
	}
	
	return count;
}

DiskDriver* disk_get(uint32_t id) {
	return map_get(disks, (void*)(uintptr_t)id);
}

