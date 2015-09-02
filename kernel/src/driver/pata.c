#include <malloc.h>
#include <string.h>
#include "pata.h"
#include "disk.h"
#include "../port.h"
#include "../time.h"

typedef struct {
	uint16_t 	configuration;
	uint16_t 	cylinder_count;
	
	uint16_t 	reserved1;
	uint16_t 	head_count;
	uint16_t 	unformated_bytes_track;
	uint16_t 	unformated_bytes_sector;
	
	uint16_t 	sector_count;
	uint16_t 	inter_sector_gap;
	uint16_t 	bytes_in_phase_lock;
	uint16_t 	vendor_status_word_count;

	uint16_t 	serial_number[10];
	uint16_t 	controller_type;
	uint16_t 	buffer_size;
	uint16_t 	ecc_bytes_count;
	uint16_t 	firmware_verision[4];

	uint16_t 	model_number[20];
	uint16_t 	reserved2[13];

	uint32_t 	total_sectors;
	uint16_t 	reserved3[196];
} __attribute__ ((packed)) HDDPriv;

typedef struct {
	bool		hdd_detected;

	bool		primary;
	bool		master;
	
	/* Interrupt not used. Left for comfortability */
	volatile bool 	primary_interrupt;
	volatile bool 	secondary_interrupt;

	HDDPriv 	priv;
} HDDManager;

static uint8_t read_hdd_status(bool primary) {
	if(primary) 
		return port_in8(PORT_PRIMARY + PORT_STATUS);
	else
		return port_in8(PORT_SECONDARY + PORT_STATUS);
}

static bool wait_for_hdd(bool primary, int flag) {
	uint64_t time = cpu_tsc();

	while((cpu_tsc() - time) <= WAITTIME * cpu_ms) {
		uint8_t status = read_hdd_status(primary);

		switch(flag) {
			case STATUS_BUSY:
				if((status & flag) != STATUS_BUSY)  
					return true;
				break;

			case STATUS_READY:
				if((status & flag) == STATUS_READY) 
					return true;
				break;
			
			case STATUS_DATAREQUEST:
				if((status & flag) == STATUS_DATAREQUEST) 
					return true;
				break;

			default:
				return false;
		}

		cpu_mwait(1);
	}
	
	return false;
}
		
//static void set_hdd_interrupt(bool primary, bool flag) {
//	if(primary) 
//		hdd_manager.primary_interrupt = flag;
//	else 
//		hdd_manager.secondary_interrupt = flag;
//}

static bool read_hdd_info(bool primary, bool master, void* hdd_info) {
	// Port setting
	uint16_t port;
	if(primary) 
		port = PORT_PRIMARY;
	else
		port = PORT_SECONDARY;

	// Wait til executing command ends
	if(!wait_for_hdd(primary, STATUS_BUSY)) {
		return false;
	}

	// LBA address setting
	uint8_t drive_flag;
	if(master)
		drive_flag = DRIVEHEAD_LBA;
	else
		drive_flag = DRIVEHEAD_LBA | DRIVEHEAD_SLAVE;

	port_out8(port + PORT_DRIVEANDHEAD, drive_flag);

	// Send command and wait
	if(!wait_for_hdd(primary, STATUS_READY)) {
		return false;
	}

//	// Interrupt init
//	set_hdd_interrupt(primary, false);
	
	port_out8(port + PORT_COMMAND, CMD_IDENTIFY);

	// TODO: wait a sec for command reception	

	uint8_t status = read_hdd_status(primary);
	if((status & STATUS_ERROR) == STATUS_ERROR) {
		return false;
	}

	// Data receive
	for(int i = 0; i < 512 / 2; i++) 
		((uint16_t*)hdd_info)[i] = port_in16(port + PORT_DATA);

	// TODO: Character array swap
	return true;
}

static int read_hdd_sector(bool primary, bool master, uint32_t lba, int sector_count, unsigned char* buf) {
	// Port setting
	uint16_t port;
	if(primary) 
		port = PORT_PRIMARY;
	else
		port = PORT_SECONDARY;

	// Wait til executing command ends
	if(!wait_for_hdd(primary, STATUS_BUSY)) {
		printf("Device is busy\n");
		return -1;
	}

	// Data register setting
	port_out8(port + PORT_SECTORCOUNT, sector_count);
	port_out8(port + PORT_SECTORNUMBER, lba);
	port_out8(port + PORT_CYLINDERLSB, lba >> 8);
	port_out8(port + PORT_CYLINDERMSB, lba >> 16);

	// LBA address setting
	uint8_t drive_flag;
	if(master)
		drive_flag = DRIVEHEAD_LBA;
	else
		drive_flag = DRIVEHEAD_LBA | DRIVEHEAD_SLAVE;

	port_out8(port + PORT_DRIVEANDHEAD, drive_flag | ((lba >> 24) & 0x0F));

	// Send command and wait
	if(!wait_for_hdd(primary, STATUS_READY)) {
		printf("Device not ready\n");
		return -2;
	}

//	// Interrupt init
//	set_hdd_interrupt(primary, false);
	
	port_out8(port + PORT_COMMAND, CMD_READ);

	// Wait for interrupt, and start to receive 
	uint8_t status;
	int read_count = 0;
	int i;
	for(i = 0; i < sector_count; i++) {
		status = read_hdd_status(primary);

		if((status & STATUS_ERROR) == STATUS_ERROR) {
			printf("Status error\n");
			return -3;
		}

		if(!wait_for_hdd(primary, STATUS_DATAREQUEST)) {
			printf("Request error\n");
			return -4;
		}

		// Read one sector
		for(int j = 0; j < 512 / 2; j++) {
			((uint16_t*)buf)[read_count++] = port_in16(port + PORT_DATA);
//			if( (uintptr_t)&buf[read_count] >0x42000)
//				printf("%p \n", &buf[read_count]);
		}
	}

	return i;
}

static int write_hdd_sector(bool primary, bool master, uint32_t lba, int sector_count, unsigned char* buf) {

	// Port setting
	uint16_t port;
	if(primary) 
		port = PORT_PRIMARY;
	else
		port = PORT_SECONDARY;
	
	// Wait til executing command ends
	if(!wait_for_hdd(primary, STATUS_BUSY)) {
		return false;
	}

	// Data register setting
	port_out8(port + PORT_SECTORCOUNT, sector_count);
	port_out8(port + PORT_SECTORNUMBER, lba);
	port_out8(port + PORT_CYLINDERLSB, lba >> 8);
	port_out8(port + PORT_CYLINDERMSB, lba >> 16);
	
	// LBA address setting
	uint8_t drive_flag;
	if(master)
		drive_flag = DRIVEHEAD_LBA;
	else
		drive_flag = DRIVEHEAD_LBA | DRIVEHEAD_SLAVE;

	port_out8(port + PORT_DRIVEANDHEAD, drive_flag | ((lba >> 24) & 0x0F));

	// Send command and wait
	if(!wait_for_hdd(primary, STATUS_READY)) {
		return false;
	}
	
	port_out8(port + PORT_COMMAND, CMD_WRITE);

	uint8_t status;
	while(1) {
		status = read_hdd_status(primary);

		if((status & STATUS_ERROR) == STATUS_ERROR) {
			return 0;
		}

		if((status & STATUS_DATAREQUEST) == STATUS_DATAREQUEST) {
			break;
		}
	}

	// Data receive and wait for interrupt
	int read_count = 0;
	int i;
	for(i = 0; i < sector_count; i++) {
		//set_hdd_interrupt(primary, false);

		for(int j = 0; j < 512 / 2; j++) {
			port_out16(port + PORT_DATA, ((uint16_t*)buf)[read_count++]);
		}

		status = read_hdd_status(primary);

		if((status & STATUS_ERROR) == STATUS_ERROR) {
			return i;
		}

		if((status & STATUS_DATAREQUEST) != STATUS_DATAREQUEST) {
			return false;
		}

	}

	return i;
}
static int pata_read(DiskDriver* driver, uint32_t lba, int sector_count, unsigned char* buf) {
	HDDManager* manager = (HDDManager*)driver->priv;

	return read_hdd_sector(manager->primary, manager->master, lba, sector_count, buf);
}

static int pata_write(DiskDriver* driver, uint32_t lba, int sector_count, unsigned char* buf) {
	HDDManager* manager = (HDDManager*)driver->priv;

	// Check device is writable
	if(!(sector_count <= 0 || sector_count > 256) 
		|| (lba + sector_count >= manager->priv.total_sectors)) {
		return 0;
	}

	return write_hdd_sector(manager->primary, manager->master, lba, sector_count, buf);
}

static int pata_init(DiskDriver* driver, DiskDriver** disks) {
	// Disable interrupt
	port_out8(PORT_PRIMARY + PORT_DIGITALOUTPUT, 2);
	port_out8(PORT_SECONDARY + PORT_DIGITALOUTPUT, 2);

	// Read HDD information
	HDDPriv priv[PATA_MAX_DRIVERS];
	HDDManager* manager[PATA_MAX_DRIVERS];

	bool primary = true, master = true;
	int count = 0;

	for(int i = 0; i < PATA_MAX_DRIVERS; i++) {
		if(read_hdd_info(primary, master, &priv[i]) == true) {
			manager[i] = malloc(sizeof(HDDManager));
			if(!manager[i]) 
				return -1; // Memory allocation fail

			memcpy(&manager[i]->priv, &priv[i], sizeof(HDDPriv));

			manager[i]->primary = primary;
			manager[i]->master = master;

			count++;
		}
		master ^= true;
		if(i % 2 == 0)
			primary ^= true;
	}

	if(count == 0) 
		return -2; // HDD not detected

	for(int i = 0; i < count; i++) {
		disks[i] = malloc(sizeof(DiskDriver));
		if(!disks[i]) 
			return -1; // Memory allocation fail
	}
	
	// Function pointer copy
	for(int i = 0; i < count; i++) {
		memcpy(disks[i], driver, sizeof(DiskDriver));

		// Private data setting
		disks[i]->number = i;
		disks[i]->priv = manager[i];
	}

	return count;
}

const DiskDriver pata_driver = {
	.type = DISK_TYPE_PATA,
	.init = pata_init,
	.read = pata_read,
	.write = pata_write, 
};

