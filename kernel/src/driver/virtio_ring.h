#ifndef __VIRTIO_RING_H__
#define __VIRTIO_RING_H__

typedef struct {
	/* Address (guest-physical). */
	uint64_t addr;
	/* Length */
	uint32_t len;

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT		1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE		2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT		4
/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC	28

	/*The flags as indicated above. */
	uint16_t flags;
	/* Next field if flags & NEXT */
	uint16_t next;
} VringDesc;

typedef struct {
#define VRING_AVAIL_F_NO_INTERRUPT	1
	uint16_t flags;
	uint16_t idx;
//	uint16_t used_event; /* Only if VIRTIO_RING_F_EVENT_IDX */
	uint16_t ring[ /* Queue Size */ ];
} VringAvail;

typedef struct {
	/* Index of start of used descriptor chain. */
	uint32_t id;
	/* Total length of the descriptor chain which was used (written to) */
	uint32_t len;
} VringUsedElem;

typedef struct {
#define VRING_USED_F_NO_NOTIFY		1
	uint16_t flags;
	uint16_t idx;
//	uint16_t avail_event; /* Only if VIRTIO_RING_F_EVENT_IDX */
	VringUsedElem ring [ /* Queue Size */ ];
} VringUsed;

typedef struct {
	uint32_t num;

	VringDesc* desc;
	VringAvail* avail;
	VringUsed* used;
} Vring;

static inline void vring_init(Vring* vr, uint32_t num, void* p, uint64_t align){
	vr->num = num;
	vr->desc = p;
	vr->avail = p + num*sizeof(VringDesc);
	vr->used = (VringUsed*)(((uintptr_t)&vr->avail->ring[num] + sizeof(uint16_t) + (uintptr_t)align - 1) & ~((uintptr_t)align -1));
}

static inline unsigned vring_size(uint32_t num, uint64_t align){
	return ((sizeof(VringDesc)*num + sizeof(uint16_t)*(3 + num)
		+ align - 1) & ~(align - 1))
		+ sizeof(uint16_t)*3 + sizeof(VringUsedElem)*num;
}

static inline int vring_need_event(uint16_t event_idx, uint16_t new_idx, uint16_t old_idx){
	return (uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old_idx);
}

/* Get location of event indices (only with VIRTIO_RING_F_EVENT_IDX) */
static inline uint16_t* vring_used_event(Vring* vr){
	/* For backwards compat, used event index is at *end* of avail ring. */
	return &vr->avail->ring[vr->num];
}

static inline uint16_t* vring_avail_event(Vring* vr){
	/* For backwards compat, avail event index is at *end* of used ring. */
	return (uint16_t*)&vr->used->ring[vr->num];
}

#endif
