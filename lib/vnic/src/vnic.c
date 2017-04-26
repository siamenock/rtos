//#include <errno.h>
#include <string.h>
#include <lock.h>
#include <vnic.h>
#include <nic.h>

#define DEBUG		1
#define ALIGN		16

/**
 *
 * Error: errno
 * 1: mandatory attributes not specified
 * 2: Conflict attributes speicifed
 * 3: Pool size is not multiple of 2MB
 * 4: Not enough memory
 */

inline uint64_t timer_frequency() {
	uint64_t time;
	uint32_t* p = (uint32_t*)&time;
	asm volatile("rdtsc" : "=a"(p[0]), "=d"(p[1]));
	
	return time;
}

static uint64_t vnic_id;

static int nic_init(uint64_t mac, void* base, size_t size,
		uint64_t rx_bandwidth, uint64_t tx_bandwidth,
		uint16_t padding_head, uint16_t padding_tail,
		uint32_t rx_queue_size, uint32_t tx_queue_size,
		uint32_t srx_queue_size, uint32_t stx_queue_size) {

	if((uintptr_t)base == 0 || (uintptr_t)base % 0x200000 != 0)
		return 1;

	if(size % 0x200000 != 0)
		return 2;

	int index = sizeof(NIC);

	NIC* nic = base;
	nic->magic = NIC_MAGIC_HEADER;
	nic->id = vnic_id++;
	nic->mac = mac;
	nic->rx_bandwidth = rx_bandwidth;
	nic->tx_bandwidth = tx_bandwidth;
	nic->padding_head = padding_head;
	nic->padding_tail = padding_tail;

	nic->rx.base = index;
	nic->rx.head = 0;
	nic->rx.tail = 0;
	nic->rx.size = rx_queue_size;
	nic->rx.rlock = 0;
	nic->rx.wlock = 0;

	index += nic->rx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->tx.base = index;
	nic->tx.head = 0;
	nic->tx.tail = 0;
	nic->tx.size = tx_queue_size;
	nic->tx.rlock = 0;
	nic->tx.wlock = 0;

	index += nic->tx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->srx.base = index;
	nic->srx.head = 0;
	nic->srx.tail = 0;
	nic->srx.size = srx_queue_size;
	nic->srx.rlock = 0;
	nic->srx.wlock = 0;

	index += nic->srx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->stx.base = index;
	nic->stx.head = 0;
	nic->stx.tail = 0;
	nic->stx.size = stx_queue_size;
	nic->stx.rlock = 0;
	nic->stx.wlock = 0;

	index += nic->stx.size * sizeof(uint64_t);
	index = ROUNDUP(index, 8);
	nic->pool.bitmap = index;
	index += (size - index) / NIC_CHUNK_SIZE;
	index = ROUNDUP(index, NIC_CHUNK_SIZE);
	nic->pool.pool = index;
	nic->pool.count = (size - index) / NIC_CHUNK_SIZE;
	nic->pool.index = 0;
	nic->pool.used = 0;
	nic->pool.lock = 0;

	nic->config = 0;
	bzero(nic->config_head, (size_t)((uintptr_t)nic->config_tail - (uintptr_t)nic->config_head));

	bzero(base + nic->pool.bitmap, nic->pool.count);

	return 0;
}

bool vnic_init(VNIC* vnic, uint64_t* attrs) {
	bool has_key(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != VNIC_NONE) {
			if(attrs[i * 2] == key)
				return true;

			i++;
		}

		return false;
	}

	uint64_t get_value(uint64_t key) {
		int i = 0;
		while(attrs[i * 2] != VNIC_NONE) {
			if(attrs[i * 2] == key)
				return attrs[i * 2 + 1];

			i++;
		}

		return (uint64_t)-1;
	}

	//TODO check key
	if(!has_key(VNIC_DEV) || !has_key(VNIC_MAC) || !has_key(VNIC_POOL_SIZE) ||
			!has_key(VNIC_RX_BANDWIDTH) || !has_key(VNIC_TX_BANDWIDTH) ||
			!has_key(VNIC_PADDING_HEAD) || !has_key(VNIC_PADDING_TAIL)) {
		return false;
	}

	nic_init(get_value(VNIC_MAC), vnic->nic, get_value(VNIC_POOL_SIZE),
			get_value(VNIC_RX_BANDWIDTH), get_value(VNIC_TX_BANDWIDTH),
			get_value(VNIC_PADDING_HEAD), get_value(VNIC_PADDING_TAIL),
			get_value(VNIC_RX_QUEUE_SIZE), get_value(VNIC_TX_QUEUE_SIZE),
			get_value(VNIC_SLOW_RX_QUEUE_SIZE), get_value(VNIC_SLOW_TX_QUEUE_SIZE));

	vnic->magic = vnic->nic->magic;
	vnic->id = vnic->nic->id;
	vnic->mac = vnic->nic->mac;
	vnic->rx_bandwidth = vnic->nic->rx_bandwidth;
	vnic->tx_bandwidth = vnic->nic->tx_bandwidth;
	vnic->padding_head = vnic->nic->padding_head;
	vnic->padding_tail = vnic->nic->padding_tail;
	vnic->rx = vnic->nic->rx;
	vnic->tx = vnic->nic->tx;
	vnic->srx = vnic->nic->srx;
	vnic->stx = vnic->nic->stx;

	//TODO
// 	vnic->min_buffer_size = 128;
// 	vnic->max_buffer_size = 2048;

	vnic->rx_closed = vnic->tx_closed = timer_frequency();

	return true;
}

uint32_t vnic_update(VNIC* vnic, uint64_t* attrs) {
	//TODO
	return 0;
}

//TODO fix name
bool vnic_rx_available(VNIC* vnic) {
	vnic->rx.head = vnic->nic->rx.head;
	return queue_available(&vnic->rx);
}

//TODO return error number
bool vnic_rx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	uint64_t time = timer_frequency();
	if(vnic->rx_closed - vnic->rx_wait_grace > time) {
		return false;
	}

	//TODO strict check 
	lock_lock(&vnic->nic->rx.wlock);
	vnic->rx.head = vnic->nic->rx.head;
	if(queue_available(&vnic->rx)) {
		Packet* packet = nic_alloc(vnic->nic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&vnic->nic->rx.wlock);
			return false;
		}

		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);

		packet->end = packet->start + size1 + size2;

		if(queue_push(vnic->nic, &vnic->rx, packet)) {
			vnic->nic->rx.tail = vnic->rx.tail;
			lock_unlock(&vnic->nic->rx.wlock);

			if(vnic->rx_closed > time)
				vnic->rx_closed += vnic->rx_wait * (size1 + size2);
			else
				vnic->rx_closed = time + vnic->rx_wait * (size1 + size2);

			return true;
		} else {
			lock_unlock(&vnic->nic->rx.wlock);
			nic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&vnic->nic->rx.wlock);
		return false;
	}
}

bool vnic_rx2(VNIC* vnic, Packet* packet) {
	uint64_t time = timer_frequency();
	if(vnic->rx_closed - vnic->rx_wait_grace > time) {
		return false;
	}
	//TODO try_lock
	lock_lock(&vnic->nic->rx.wlock);
	vnic->rx.head = vnic->nic->rx.head;
	if(queue_push(vnic->nic, &vnic->rx, packet)) {
		vnic->nic->rx.tail = vnic->rx.tail;
		lock_unlock(&vnic->nic->rx.wlock);

		if(vnic->rx_closed > time)
			vnic->rx_closed += vnic->rx_wait * (packet->end - packet->start);
		else
			vnic->rx_closed = time + vnic->rx_wait * (packet->end - packet->start);

		return true;
	} else {
		lock_unlock(&vnic->nic->rx.wlock);
		nic_free(packet);
		return false;
	}
}

bool vnic_has_srx(VNIC* vnic) {
	vnic->srx.head = vnic->nic->srx.head;
	return queue_available(&vnic->srx);
}

bool vnic_srx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2) {
	lock_lock(&vnic->nic->srx.wlock);
	vnic->srx.head = vnic->nic->srx.head;
	if(queue_available(&vnic->srx)) {
		Packet* packet = nic_alloc(vnic->nic, size1 + size2);
		if(packet == NULL) {
			lock_unlock(&vnic->nic->srx.wlock);
			return false;
		}

		memcpy(packet->buffer + packet->start, buf1, size1);
		memcpy(packet->buffer + packet->start + size1, buf2, size2);

		packet->end = packet->start + size1 + size2;

		if(queue_push(vnic->nic, &vnic->srx, packet)) {
			vnic->nic->srx.tail = vnic->srx.tail;
			lock_unlock(&vnic->nic->srx.wlock);
			return true;
		} else {
			lock_unlock(&vnic->nic->srx.wlock);
			nic_free(packet);
			return false;
		}
	} else {
		lock_unlock(&vnic->nic->srx.wlock);
		return false;
	}
}

bool vnic_srx2(VNIC* vnic, Packet* packet) {
	//TODO try_lock
	lock_lock(&vnic->nic->srx.wlock);
	vnic->srx.head = vnic->nic->srx.head;
	if(queue_push(vnic->nic, &vnic->srx, packet)) {
		vnic->nic->srx.tail = vnic->srx.tail;
		lock_unlock(&vnic->nic->srx.wlock);
		return true;
	} else {
		lock_unlock(&vnic->nic->srx.wlock);
		nic_free(packet);
		return false;
	}
}

bool vnic_has_tx(VNIC* vnic) {
	vnic->tx.tail = vnic->nic->tx.tail;
	return !queue_empty(&vnic->tx);
}

Packet* vnic_tx(VNIC* vnic) {
	//TODO check
	//TODO try lock
	uint64_t time = timer_frequency();
	if(vnic->tx_closed - vnic->tx_wait_grace > time)
		return NULL;

	lock_lock(&vnic->nic->tx.rlock);
	vnic->tx.tail = vnic->nic->tx.tail;
	Packet* packet = queue_pop(vnic->nic, &vnic->tx);

	if(packet) {
		if(vnic->tx_closed > time)
			vnic->tx_closed += vnic->tx_wait * (packet->end - packet->start);
		else
			vnic->tx_closed = time + vnic->tx_wait * (packet->end - packet->start);
	}

	vnic->nic->tx.head = vnic->tx.head;
	lock_unlock(&vnic->nic->tx.rlock);

	return packet;
}

bool vnic_has_stx(VNIC* vnic) {
	vnic->stx.tail = vnic->nic->stx.tail;
	return !queue_empty(&vnic->stx);
}

Packet* vnic_stx(VNIC* vnic) {
	//TODO try lock
	lock_lock(&vnic->nic->stx.rlock);
	vnic->stx.tail = vnic->nic->stx.tail;
	Packet* packet = queue_pop(vnic->nic, &vnic->stx);
	vnic->nic->stx.head = vnic->stx.head;
	lock_unlock(&vnic->nic->stx.rlock);

	return packet;
}

