#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/icmp.h>

#include <tlsf.h>

#include <_malloc.h>
#include <lock.h>
#include <util/fifo.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/ip.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define POOL_SIZE	0x40000

extern int __nic_count;
extern NIC* __nics[NIC_SIZE];

static void icmp_process_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(2, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);

	Packet* packet = nic_alloc(__nics[0], 256);
	
	// packet ether type is not set.
	assert_false(icmp_process(packet));


	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ether->type = endian16(ETHER_TYPE_IPv4);

	// nic_ip_get fail.
	assert_false(icmp_process(packet));

	IP* ip = (IP*)ether->payload;
	ip->destination = 0x12345678;
	uint32_t addr = endian32(ip->destination);

	nic_ip_add(__nics[0], addr);

	// protocal is not set.
	assert_false(icmp_process(packet));

	ip->protocol = IP_PROTOCOL_ICMP;

	nic_output(__nics[0], packet);
	assert_true(icmp_process(packet));

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(icmp_process_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
