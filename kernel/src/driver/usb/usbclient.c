/*
 * This file is part of FILO.
 *
 * Copyright (C) 2008 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "libpayload-config.h"

/* Only use this code if libpayload is compiled with USB stack */
#ifdef CONFIG_LP_USB
#include "usb.h"
#include "usbmsc.h"
#include "../disk.h"  
#include "../fs.h"

typedef uint64_t sector_t;

// FIXME: should be dynamic?
#define maxdevs 4
static usbdev_t* devs[maxdevs];
static int count = -1;

void usbdisk_create (usbdev_t* dev)
{
	if (count == maxdevs-1) return;
	devs[++count] = dev;
}

void usbdisk_remove (usbdev_t* dev)
{
	int i;
	if (count == -1) return;
	if (devs[count] == dev) {
		count--;
		return;
	}
	for (i=0; i<count; i++) {
		if (devs[i] == dev) {
			devs[i] = devs[count];
			count--;
			return;
		}
	}
}

int usb_read(usbdev_t* dev, const sector_t sector, const int sector_count, void *buffer)
{
	int result = -readwrite_blocks_512(dev, sector, sector_count, cbw_direction_data_in, buffer);
	return result;
}

/* These fucntions added for PacketNgin */
static int usb_client_init(DiskDriver* driver, DiskDriver** disks) {
	usb_poll();
	
	int i;
	usbdev_t* dev = NULL;
	for(i = 0; i < maxdevs; i++) {
		if(devs[i] != NULL) {
			dev = devs[i];
			break;
		}
	}

	if(dev == NULL)
		return -1; // Driver table is full

	/* FIXME: need a place to periodically poll usb_poll().
	   or at least at sensible times.
	   this would be a nice place, but the usb disk handling
	   must be more clever for that.
	 */
	if(count >= i) {
		// New device detected
		int new = count - i + 1; 
		
		for(int j = 0; j < new; j++) {
			disks[j] = malloc(sizeof(DiskDriver));
			if(!disks[j]) 
				return -2; // Memory allocation fail
		}

		// Function pointer copy
		for(int j = 0; j < new; j++) {
			memcpy(disks[j], driver, sizeof(DiskDriver));

			// Private data setting
			disks[j]->number = j;
			disks[j]->priv = devs[j];
		}

		return new;
	}

	return -2; // USB MSC not detected
}

static int usb_msc_read(DiskDriver* driver, uint32_t lba, int sector_count, unsigned char* buf) {
	int result = usb_read(driver->priv, lba, sector_count, buf);

	// 0 returned if reading success
	if(!result) 
		result = FS_SECTOR_SIZE * sector_count;
	else	
		result = 0;

	return result;
}

DiskDriver usb_msc_driver = {
	.type = DISK_TYPE_USB,
	.init = usb_client_init,
	.read = usb_msc_read,
};

#endif
