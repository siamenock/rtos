#ifndef __DISK_H__
#define __DISK_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DISK_MAX_DRIVERS	0x10

#define DISK_TYPE_NULL 		0x00
#define DISK_TYPE_PATA		0x01
#define DISK_TYPE_SATA		0x02
#define DISK_TYPE_USB		0x03

#define DISK_AVAIL_DEVICES	8 	/* Available disks for each driver */

typedef struct _DiskDriver DiskDriver; 

typedef struct _DiskDriver {
	// Driver information
	uint16_t type;
	uint16_t number;
	
	/* Low level file operation - disk specific */
	int	(*init)(DiskDriver* driver, DiskDriver** disks);
	int	(*read)(DiskDriver* driver, uint32_t lba, int sector_count, unsigned char* buf);
	int	(*write)(DiskDriver* driver, uint32_t lba, int sector_count, unsigned char* buf);

	// Probe information
	void* 	priv;
} DiskDriver;

bool disk_init0();
bool disk_register(const DiskDriver* driver);
bool disk_init();
size_t disk_count();
int disk_ids(uint32_t* ids, int size);
DiskDriver* disk_get(uint32_t id);

#endif /* __DISK_H__ */
