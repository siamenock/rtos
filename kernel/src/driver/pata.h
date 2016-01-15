#ifndef __PATA_H__
#define __PATA_H__

#include "disk.h"

#define PATA_MAX_DRIVERS		0x04

#define PORT_PRIMARY			0x01f0
#define PORT_SECONDARY			0x0170

#define PORT_DATA			0x00
#define PORT_SECTORCOUNT		0x02
#define PORT_SECTORNUMBER		0x03
#define PORT_CYLINDERLSB		0x04
#define PORT_CYLINDERMSB		0x05
#define PORT_DRIVEANDHEAD		0x06
#define PORT_STATUS			0x07
#define PORT_COMMAND			0x07
#define PORT_DIGITALOUTPUT		0x0206

#define CMD_READ			0x20
#define CMD_WRITE			0x30
#define CMD_IDENTIFY			0xec

#define STATUS_ERROR			0x01
#define STATUS_INDEX			0x02
#define STATUS_CORRECTEDDATA		0x04
#define STATUS_DATAREQUEST		0x08
#define STATUS_SEEKCOMPLETE		0x10
#define STATUS_WRITEFAULT		0x20
#define STATUS_READY			0x40
#define STATUS_BUSY			0x80

#define DRIVEHEAD_LBA			0xe0
#define DRIVEHEAD_SLAVE			0x10

#define DIGITALOUTPUT_RESET		0x04
#define DIGITALOUTPUT_DISABLEINTERRUPT	0x01

#define WAITTIME			500 	// 500ms

extern DiskDriver pata_driver;

#endif /* __PATA_H__ */
