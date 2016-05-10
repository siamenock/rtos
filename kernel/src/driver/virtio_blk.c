#include <malloc.h>
#include <string.h>

// PacketNgin kernel header
#include <util/list.h>
#include <util/event.h>
#include "disk.h"
#include "../file.h"
#include "../gmalloc.h"
#include "../port.h"
#include "../cpu.h"
#include "../pci.h"
#include "../page.h"

// Virtio driver header
#include "virtio_ring.h"
#include "virtio_blk.h"
#include "virtio_pci.h"
#include "virtio_config.h"

#define PORTIO(bus, slot, function, reg)	((1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | (reg & 0xfc))

//static bool event(void* data); 

typedef struct {
	VirtQueue* vq_blk;
} VirtBlkPriv;

/* Private structure that virtio block device driver uses */
static VirtBlkPriv* priv;

typedef struct {
	void(*callback)(List* blocks, int count, void* context);
	void*			context;
	uint64_t 		event_id;
	int			count;
	List*			blocks;
	List*			free_list;
	List*			status_list;
} EventContext;

static int blk_done(List* status_list) {
	ListIterator iter;
	list_iterator_init(&iter, status_list);
	while(list_iterator_has_next(&iter)) {
		uint8_t* status = list_iterator_next(&iter);
		if(*status == 0xff)
			return 0;
		else if(*status == VIRTIO_BLK_S_IOERR) {
			return -VIRTIO_BLK_S_IOERR;
		}
		else if(*status == VIRTIO_BLK_S_UNSUPP) {
			return -VIRTIO_BLK_S_UNSUPP;
		}
	}

	return 1;
}

static void request_gfree(List* free_list) {
	ListIterator iter;
	list_iterator_init(&iter, free_list);

	while(list_iterator_has_next(&iter)) {
		void* data = list_iterator_next(&iter);
		if(data)
			gfree(data);
		list_iterator_remove(&iter);
	}

	list_destroy(free_list);
}

bool event(void* context) {
	EventContext* event_context = context;

	int err = blk_done(event_context->status_list);
	if(err) {
		VirtQueue* vq = priv->vq_blk;

		if(vq->indirect)
			vq->num_free += list_size(event_context->status_list);
		else
			vq->num_free += list_size(event_context->status_list) * 3;

		request_gfree(event_context->free_list);
		list_destroy(event_context->status_list);

		if(err == 1)
			err = event_context->count;

		event_context->callback(event_context->blocks, err, event_context->context);

		free(event_context);

		return false;
	}

	return true;
}

int add_event(List* blocks, List* free_list, List* status_list, int count, void(*callback)(List* blocks, int count, void* context), void* context) {
	EventContext* event_context = malloc(sizeof(EventContext));
	event_context->callback = callback;
	event_context->context = context;
	event_context->count = count;
	event_context->blocks = blocks;
	event_context->free_list = free_list;
	event_context->status_list = status_list;

	event_context->event_id = event_busy_add(event, event_context);
	
	return event_context->event_id;
}

static int add_request(VirtQueue* vq, VirtIOBlkReq* req, List* reqs) {
	if(vq->num_free == 0) {
		return -FILE_ERR_NOSPC;
	}

	if(!vq->indirect)
		vq->num_free -= 3;

	list_add(reqs, req);

	return true;
}

static VirtIOBlkReq* buf_to_req(void* buffer, uint32_t type, uint32_t sector, int sector_count) {
	VirtIOBlkReq* req = gmalloc(sizeof(VirtIOBlkReq));
	req->type = type;
	req->status = 0xff;
	req->data = (void*)VIRTUAL_TO_PHYSICAL((uintptr_t)buffer);
	req->sector = sector;
	req->sector_count = sector_count;
	return req;
}

static int block_op(void* buffer, uint32_t type, uint32_t sector, int sector_count) {
	VirtQueue* vq = priv->vq_blk;
	List* req_list = list_create(NULL);

	VirtIOBlkReq* req = buf_to_req(buffer, type, sector, sector_count);

	if(add_request(vq, req, req_list) == -FILE_ERR_NOSPC) {
		gfree(req);
		list_destroy(req_list);
		return -FILE_ERR_NOSPC;
	}

	List* free_list = list_create(NULL);
	List* status_list = list_create(NULL);
	vq->vq_ops->add_buf(vq, req_list, free_list, status_list);
	vq->vq_ops->kick(vq);

	while(1) {
		int err = blk_done(status_list);
		if(err) {
			if(vq->indirect)
				vq->num_free += list_size(status_list);
			else
				vq->num_free += 3 * list_size(status_list);

			request_gfree(free_list);

			list_destroy(req_list);
			list_destroy(status_list);
			return err;
		}
	}

	return 1;
}

/* Function to Operate the read command */
int virtio_blk_read(DiskDriver* driver, uint32_t sector, int sector_count, uint8_t* buf) {
	return block_op(buf, VIRTIO_BLK_T_IN, sector, sector_count);
}

/* Function to Operate the write command */
int virtio_blk_write(DiskDriver* driver, uint32_t sector, int sector_count, uint8_t* buf) {
	return block_op(buf, VIRTIO_BLK_T_OUT, sector, sector_count);
}

/* Function to Operate the read command */
int virtio_blk_read_async(DiskDriver* driver, List* blocks, int sector_count, void(*callback)(List* blocks, int count, void* context), void* context) {
	VirtQueue* vq = priv->vq_blk;
	List* req_list = list_create(NULL);
	List* free_list = list_create(NULL);
	List* status_list = list_create(NULL);

	int count = 0;
	int pre_sector;
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		BufferBlock* block = list_iterator_next(&iter);

		// Check if buffer was from cache or not
		if(!block->buffer) {
			block->buffer = gmalloc(512 * sector_count);
			VirtIOBlkReq* req = buf_to_req(block->buffer, VIRTIO_BLK_T_IN, block->sector, sector_count);

			// When we use indirect
			if(vq->indirect) {
				// If the request list is full
				if(list_size(req_list) > 100 || (list_size(req_list) > 0 && pre_sector != req->sector - 8))
					vq->vq_ops->add_buf(vq, req_list, free_list, status_list);

				pre_sector = req->sector;
			}

			// Add the request into a request list
			if(add_request(vq, req, req_list) == -FILE_ERR_NOSPC) {
				gfree(block->buffer);
				gfree(req);
				break;
			}
		} 
	
		count++;
	}

	// If there's a request that hasn't been put to buffer
	if(list_size(req_list) > 0)
		vq->vq_ops->add_buf(vq, req_list, free_list, status_list);

	// If there's a request that should be checked
	if(list_size(free_list) > 0) 
		vq->vq_ops->kick(vq);

	// We don't need it anymore.
	list_destroy(req_list);

	add_event(blocks, free_list, status_list, count, callback, context);

	return 0;
}

/* Function to Operate the write command */
int virtio_blk_write_async(DiskDriver* driver, List* blocks, int sector_count, void(*callback)(List* blocks, int count, void* context), void* context) {
	VirtQueue* vq = priv->vq_blk;
	List* req_list = list_create(NULL);
	List* free_list = list_create(NULL);
	List* status_list = list_create(NULL);

	int len = 0;
	int pre_sector;
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		BufferBlock* block = list_iterator_next(&iter);

		VirtIOBlkReq* req = buf_to_req(block->buffer, VIRTIO_BLK_T_OUT, block->sector, sector_count);

		// When we use indirect
		if(vq->indirect) {
			// If the request list is full
			if(list_size(req_list) > 100 || (list_size(req_list) > 0 && pre_sector != req->sector - 8))
				vq->vq_ops->add_buf(vq, req_list, free_list, status_list);

			pre_sector = req->sector;
		}

		// Add the request into a request list
		if(add_request(vq, req, req_list) == -FILE_ERR_NOSPC) {
			gfree(req);
			break;
		}

		len += block->size;
		list_iterator_remove(&iter);
		free(block);
	}

	// If there's a request that hasn't been put to buffer
	if(list_size(req_list) > 0)
		vq->vq_ops->add_buf(vq, req_list, free_list, status_list);

	// We don't need it anymore.
	list_destroy(req_list);

	// If there's a request that should be checked
	if(list_size(free_list) > 0) {
		vq->vq_ops->kick(vq);
		add_event(blocks, free_list, status_list, len, callback, context);
	} else {
		list_destroy(free_list);
		list_destroy(status_list);

		callback(blocks, len, context);
	}

	return list_size(blocks);
}

/* Add status to virtio configuration status space */
static void add_status(VirtIODevice* vdev, uint8_t status) {
	vdev->status |= status;
	port_out8(vdev->ioaddr + VIRTIO_PCI_STATUS, vdev->status);
}

/* Probing function for PCI device */
static int virtio_pci_probe(VirtIODevice* vdev) {

	// Read I/O address
	vdev->ioaddr = pci_read32(vdev->dev, PCI_BASE_ADDRESS_0) & ~3;

	if(!vdev->ioaddr)
		return -1;

	// Enable device
	pci_enable(vdev->dev);
	
	// Reset device 
	add_status(vdev, 0);

	// OS notice device from now
	add_status(vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE);

	// OS notice driver from now
	add_status(vdev, VIRTIO_CONFIG_S_DRIVER);
	
	return 0;
}

/* Synchronize and determine features with host */
static int synchronize_features(VirtIODevice* vdev) {
	uint64_t device_features;
	uint64_t driver_features;

	static unsigned int features[] = {
		VIRTIO_BLK_F_SEG_MAX, VIRTIO_BLK_F_SIZE_MAX, VIRTIO_BLK_F_GEOMETRY,
		VIRTIO_BLK_F_RO, VIRTIO_BLK_F_BLK_SIZE,
		VIRTIO_BLK_F_TOPOLOGY, VIRTIO_RING_F_INDIRECT_DESC,
	};

	// Figure out what features device supports
	device_features = port_in32(vdev->ioaddr + VIRTIO_PCI_HOST_FEATURES);

	// Features supported by both device and driver
	uint32_t fbit;
	driver_features = 0;

	for(size_t i = 0; i < sizeof(features)/sizeof(int); i++) {
		fbit = features[i];
		if(fbit > 32) {
			printf("We only support 32 feature bits\n");
			return -1;
		}
		driver_features |= (1ULL << fbit);
	}
	driver_features &= device_features;
	
	// Finally determine features that we are going to use
	port_out32(vdev->ioaddr + VIRTIO_PCI_GUEST_FEATURES, driver_features);
	vdev->features |= driver_features;

	add_status(vdev, VIRTIO_CONFIG_S_FEATURES_OK);

	/* Device is alive at this point */
	add_status(vdev, VIRTIO_CONFIG_S_DRIVER_OK);

	return 0;
}

/* Initializing function for virtqueues */
int init_vq(VirtQueue* vq) {

	extern VirtQueueOps vops;
	uint32_t ioaddr;

	if(device_has_feature(vq->vdev, VIRTIO_RING_F_INDIRECT_DESC)) {
		vq->indirect = true;
	}
	vq->vq_ops = gmalloc(sizeof(VirtQueueOps));
	vq->vq_ops = &vops;

	/* Select the queue we're interested in
	 * Virtio block driver uses only one queue */
	ioaddr = vq->vdev->ioaddr;
	port_out16(ioaddr + VIRTIO_PCI_QUEUE_SEL, 0);

	// Check if queue is either not available or already active
	int num = port_in32(ioaddr + VIRTIO_PCI_QUEUE_NUM);
	if(!num || port_in32(ioaddr + VIRTIO_PCI_QUEUE_PFN))
		return -1;	

	// Memory allocation for vring area
	void* queue = gmalloc(0x2000 + 0xfff);
	if(!queue) {
		printf("Queue malloc failed\n");
		return -3;
	}
	/* The last 3 address should be aligned in 0.
	 * Because device recognize the address in that way. */
	queue = (void*)((uintptr_t)(queue+0xfff) & ~0xfff);
	memset(queue, 0, 0x2000);

	// Activate the queue
	port_out32(ioaddr + VIRTIO_PCI_QUEUE_PFN, (uintptr_t)queue >> VIRTIO_PCI_QUEUE_ADDR_SHIFT);

	// Create the vring
	vring_init(vq->vring, num, queue, VIRTIO_PCI_VRING_ALIGN);
	
	// Initialize virtqueue
	vq->num_free = num - (num % 3);
	vq->num_added = 0;
	vq->last_used_idx = 0;

	// We don't have interrupt handler. Tell otherside not to interrupt us
	vq->vring->avail->flags |= VRING_AVAIL_F_NO_INTERRUPT;
	vq->vring->used->flags |= VRING_USED_F_NO_NOTIFY;

	// Put everything in free lists
	vq->free_head = 0;

	// Every descriptor field indicates the next one
	for(int i = 0; i < num; i++) {
		vq->vring->desc[i].next = i + 1;
	}
	vq->vring->desc[num-1].next = 0;
	return 0;
}

DeviceType virtio_device_type = DEVICE_TYPE_VIRTIO_BLK;

int virtio_pci_init(void* device, void* data) {
	return 0;
}

void virtio_destroy(int id) {
}

Driver virtio_pci_driver = {
	.init = virtio_pci_init,
	.destroy = virtio_destroy,
};

static PCI_ID pci_ids[] = {
	PCI_DEVICE(0x1af4, 0x1001, "virtio", NULL), 
	{ 0 }
};

bool virtio_device_probe(PCI_Device* pci, char** name, void** data) {
	for(int i = 0; pci_ids[i].vendor_id != 0; i++) {
		PCI_ID* id = &pci_ids[i];

		if(pci->vendor_id == id->vendor_id && pci->device_id == id->device_id) {
			*name = id->name;
			*data = id->data;

			priv->vq_blk->vdev->dev = pci;
			return 1;
		}
	}
	return 0;
}
static int virtio_blk_init(DiskDriver* driver, const char* cmdline, DiskDriver** disks) {
	int err, count;

	priv = gmalloc(sizeof(VirtBlkPriv));

	// Memory allocations for a virtqueue & request buffers
	priv->vq_blk = gmalloc(sizeof(VirtQueue));
	priv->vq_blk->vdev = gmalloc(sizeof(VirtIODevice));
	priv->vq_blk->vring = gmalloc(sizeof(Vring));

	count = pci_probe(virtio_device_type, virtio_device_probe, &virtio_pci_driver);
	if(!count)
		return -1;

	// Virtio device PCI probing 
	err = virtio_pci_probe(priv->vq_blk->vdev);
	if(err)
		return -2;

	// Feature synchronizing with host OS
	err = synchronize_features(priv->vq_blk->vdev);
	if(err)
		return -3;

	// Initialize virtqueue & vring
	err = init_vq(priv->vq_blk);
	if(err)
		return -4;

	// Disk attachment
	for(int i = 0; i < count; i++) {
		disks[i] = gmalloc(sizeof(DiskDriver));
		if(!disks[i])
			return -5; // Memory allocation fail

		// Function pointer copy
		memcpy(disks[i], driver, sizeof(DiskDriver));

		// Private data setting
		disks[i]->number = i;
		disks[i]->priv = priv;
	}

	return count;
}

DiskDriver virtio_blk_driver = {
	.type = DISK_TYPE_VIRTIO_BLK,
	.init = virtio_blk_init,
	.read = virtio_blk_read,
	.write = virtio_blk_write,
	.read_async = virtio_blk_read_async,
	.write_async = virtio_blk_write_async,
};
