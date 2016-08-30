#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <net/ip.h>
#include <net/ether.h>
#include <net/packet.h>

Packet* mock_nic_alloc(NIC* nic, uint16_t size) {
	Packet* packet = malloc(sizeof(Packet) + size);
	if(packet) {
		bzero(packet, sizeof(Packet));
		packet->nic = nic;
		packet->size = size;
		packet->start = 0;
		packet->end = size;
	}
	
	return packet;
}

uint8_t sample_packet[66] = {		// sample packet(66 byte)
			0x90, 0x9f, 0x33, 0xa3, 0xc5, 0x70, 0x10, 0xc3, 
			0x7b, 0x93, 0x09, 0xd1, 0x08, 0x00, 0x45, 0x00, 
			0x00, 0x34,	0x0d, 0x76, 0x40, 0x00, 0x40, 0x06,
			0x9e, 0xd5, 0xc0, 0xa8, 0x0a, 0x6f, 0x60, 0x10,
			0x63, 0x51, 0xba, 0xba, 0x00, 0x50, 0xc3, 0x69,
			0x81, 0xf8, 0x56, 0x8a, 0x7e, 0xa6, 0x80, 0x10,
			0x00, 0xe5, 0x65, 0xd8, 0x00, 0x00, 0x01, 0x01,
			0x08, 0x0a, 0x00, 0xd0, 0x8d, 0x8b, 0xac, 0x97,
			0x70, 0xf6
};

static void ip_pack_func(void** state) {
	Packet* packet = mock_nic_alloc(NULL, 66);
	memcpy(packet->buffer, sample_packet, sizeof(sample_packet));
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	
	uint16_t comp_len = ip->length;
	uint8_t comp_ttl = ip->ttl;
	uint16_t comp_checksum = ip->checksum;
	uint16_t comp_end = packet->end;

	/*	initialization	*/
	ip->length = 0;
	ip->ttl = 0;
	ip->checksum = 0;
	packet->end = 0;

	ip_pack(packet, 32);

	assert_int_equal(ip->length, comp_len);
	assert_int_equal(ip->ttl, comp_ttl);
	assert_int_equal(ip->checksum, comp_checksum);
	assert_int_equal(packet->end, comp_end);
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(ip_pack_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
