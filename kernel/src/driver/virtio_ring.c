#include <string.h>
#include <malloc.h>
// PacketNgin kernel header
#include <util/list.h>
#include "../gmalloc.h"
#include "../cpu.h"
#include "../port.h"
#include "../pci.h"
// Virtio driver header
#include "virtio_ring.h"
#include "virtio_blk.h"
#include "virtio_config.h"
#include "virtio_pci.h"

#define PORTIO(bus, slot, function, reg)        ((1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | (reg & 0xfc))

/* Let other side know about buffer changed */
void kick(VirtQueue* vq) {
	// Data in guest OS need to be set before we update avail ring index
	asm volatile("sfence" ::: "memory");

	vq->vring->avail->idx += vq->num_added;
	vq->num_added = 0;

	// Need to update avail index before notify otherside
	asm volatile("mfence" ::: "memory");

	// Notify the other side
	port_out16(vq->vdev->ioaddr + VIRTIO_PCI_QUEUE_NOTIFY, 0);
}

static uint8_t* add_indirect(VirtQueue* vq, List* req_list, List* free_list) {
	size_t request_num = list_size(req_list) + 2; // Data parts + head, tail
	VringDesc* desc = gmalloc(request_num * sizeof(VringDesc));
	VirtIOBlkReq* first_req;
	int count = 1;

	// Add requests into the descriptor table
	ListIterator iter;
	list_iterator_init(&iter, req_list);
	while(list_iterator_has_next(&iter)) {
		VirtIOBlkReq* req = list_iterator_next(&iter);
		if(req) {
			// We just put data parts from this iterator 
			desc[count].flags = VRING_DESC_F_NEXT;
			if(req->type == VIRTIO_BLK_T_IN)
				desc[count].flags |= VRING_DESC_F_WRITE;

			desc[count].addr = (uintptr_t)req->data;
			desc[count].len = 512 * (req->sector_count);
			desc[count].next = count + 1;

			list_iterator_remove(&iter);

			if(count == 1) 
				first_req = req;

			count++;

			if(free_list)
				list_add(free_list, req);
		}
	}

	// Now we put request head and tail parts.

	// Request header part
	desc[0].flags = VRING_DESC_F_NEXT;
	desc[0].addr = (uintptr_t)first_req;
	desc[0].len = sizeof(uint64_t) * 2;
	desc[0].next = 1;

	// Request status part
	desc[count].flags = ~VRING_DESC_F_NEXT & VRING_DESC_F_WRITE;
	desc[count].addr = (uintptr_t)&(first_req->status);
	desc[count].len = sizeof(uint8_t);
	desc[count].next = 0;

	// Now we put indirect descriptor chain into actual descriptor table.
	int head = vq->free_head;
	vq->vring->desc[head].flags = VRING_DESC_F_INDIRECT;
	vq->vring->desc[head].addr = (uintptr_t)desc;
	vq->vring->desc[head].len = (count + 1) * sizeof(VringDesc);

	// Put descriptor head index in avail ring
	int avail = (vq->vring->avail->idx + vq->num_added++) % (vq->vring->num); 
	vq->vring->avail->ring[avail] = vq->free_head;

	vq->free_head = vq->vring->desc[head].next;

	list_add(free_list, desc);

	return &(first_req->status);
}

int add_buf(VirtQueue* vq, List* req_list, List* free_list, List* status_list) {

	// If device supports indirect descriptor
	if(vq->indirect) {
		list_add(status_list, add_indirect(vq, req_list, free_list));
		vq->num_free--;
		return 0;
	}

	// Add requests into the descriptor table
	VirtIOBlkReq*  req = NULL;
	ListIterator iter;
	list_iterator_init(&iter, req_list);
	while(list_iterator_has_next(&iter)) {
		req = list_iterator_next(&iter);
		if(req) {
			int head = vq->free_head;

			// Request header part
			vq->vring->desc[head].flags = VRING_DESC_F_NEXT;
			vq->vring->desc[head].addr = (uintptr_t)req;
			vq->vring->desc[head].len = sizeof(uint64_t)*2;
			head = vq->vring->desc[head].next;

			// Request data part
			vq->vring->desc[head].flags = VRING_DESC_F_NEXT;

			// If it's read request, write flag should be written.
			if(req->type == VIRTIO_BLK_T_IN)
				vq->vring->desc[head].flags |= VRING_DESC_F_WRITE;
			vq->vring->desc[head].addr = (uintptr_t)req->data;
			vq->vring->desc[head].len = 512 * req->sector_count;
			head = vq->vring->desc[head].next;

			// Request status part
			vq->vring->desc[head].flags = ~VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
			vq->vring->desc[head].addr = (uintptr_t)&(req->status);
			vq->vring->desc[head].len = sizeof(uint8_t);

			// Put descriptor head index in avail ring
			int avail = (vq->vring->avail->idx + vq->num_added++) % (vq->vring->num); 
			vq->vring->avail->ring[avail] = vq->free_head;

			vq->free_head = vq->vring->desc[head].next;

			// Remove a request from the list
			list_iterator_remove(&iter);

			list_add(free_list, req);
			list_add(status_list, &(req->status));
		}
	}

	return 0;
}	

/* Get used buffer which host OS used */
int get_buf(VirtQueue* vq, uint32_t* len) {

	// Data in host OS should be exposed before guest OS reads
	asm volatile("lfence" ::: "memory");

	return 1;
}

VirtQueueOps vops = { 
	.add_buf = add_buf,
	.get_buf = get_buf,
	.kick = kick,
};
