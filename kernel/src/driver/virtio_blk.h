#ifndef __VIRTIO_BLK_H__
#define __VIRTIO_BLK_H__

#include "disk.h"
#include "virtio_config.h"
#include "virtio_ring.h"
#include "../pci.h"

#define VIRTIO_BLK_F_BARRIER	0	/* Does host support barriers? */
#define VIRTIO_BLK_F_SIZE_MAX	1	/* Indicates maximum segment size */
#define VIRTIO_BLK_F_SEG_MAX	2	/* Indicates maximum # of segments */
#define VIRTIO_BLK_F_GEOMETRY	4	/* Legacy geometry available */
#define VIRTIO_BLK_F_RO		5	/* Disk is read-only */
#define VIRTIO_BLK_F_BLK_SIZE	6	/* Block size of disk is available */
#define VIRTIO_BLK_F_SCSI	7	/* Supports scsi packet command */
#define VIRTIO_BLK_F_FLUSH	9	/* Cache flush command support */
#define VIRTIO_BLK_F_TOPOLOGY	10	/* Device exports information on optimal I/O alignment */
#define VIRTIO_BLK_F_CONFIG_WCE	11	/* Device can toggle its cache between writeback and writethrough modes */

#define VIRTIO_DEV_ANY_ID	0xffffffff
#define VIRTIO_ID_BLOCK		2	

typedef struct{
	/* Common structure */
	uint32_t ioaddr;
	uint64_t features;
	uint8_t status;
	
        /* Virtio over PCI */
        PCI_Device* dev;

} VirtIODevice;


/* Check whether virtio device has specific featurs */
static inline bool device_has_feature(const VirtIODevice* vdev, uint32_t fbit) {
	if(fbit > 32) {
	//	printf("Feature bit needs to be under 32\n");
		return false;
	}

	return 1UL & (vdev->features >> fbit);
}

/* The final status byte is written by the device */
#define VIRTIO_BLK_S_OK		0	/* For success */
#define VIRTIO_BLK_S_IOERR	1	/* For device or driver error */
#define VIRTIO_BLK_S_UNSUPP	2	/* For a request unsupported by device */

#define VIRTIO_BLK_T_BARRIER	0x80000000	/* High bit indicates that this request acts as a barrier and that all preceeding request must be complete before this one, and all following requests must not be started until this is complete. */

typedef struct {
#define VIRTIO_BLK_T_IN		0	/* Type of the request for read */
#define VIRTIO_BLK_T_OUT	1	/* Type of the request for write */
#define VIRTIO_BLK_T_FLUSH	4	/* Type of the request for flush */
#define VIRTIO_BLK_T_FLUSH_OUT	5	/* Type of the request for flush */
/* The FLUSH and FLUSH_OUT types are equivalent, the device does not distinguish between them */
	uint32_t type;
	uint32_t reserved;
	int64_t sector;
	uint8_t status;
	void* data;
	int sector_count;
//	char data[][512];
} VirtIOBlkReq;

typedef struct _VirtQueue VirtQueue;

typedef struct {
        int (*add_buf)(VirtQueue *vq, List* req_list, List* free_list, List* status_list);
        void (*kick)(VirtQueue *vq); 
        int (*get_buf)(VirtQueue *vq, uint32_t* len);
        void (*disable_cb)(VirtQueue *vq);
        void (*enable_cb)(VirtQueue *vq);
} VirtQueueOps;

struct _VirtQueue {
        VirtIODevice* vdev;
        VirtQueueOps* vq_ops;
	Vring* vring;

	/* Head of free buffer list. */
	uint32_t free_head;
	/* Number we've added since last sync. */
	uint32_t num_added;
	/* Last used index we've seen. */
	uint16_t last_used_idx;

	//List* desc_list;
//	int capacity;
//	VringDesc* desc;
	
	/* Host supports indirect buffers */
	bool indirect;

	/* Number of free buffers */
	uint32_t num_free;
};

/* If the device has VIRTIO_BLK_F_SCSI feature, it can also support scsi packet command requests */
/* All fields are in guest's native endian */
extern const DiskDriver virtio_blk_driver;

/* Check whether device used avail buffer */
static inline bool hasUsedIdx(VirtQueue* vq) {
        return vq->vring->used->idx != vq->last_used_idx;
}
#endif
