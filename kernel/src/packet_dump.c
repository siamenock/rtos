#include <stdio.h>
#include "net/ether.h"
#include "util/cmd.h"
#include "driver/nicdev.h"
#include "packet_dump.h"

#define DUMP_INFO			1 << 0
#define DUMP_ETHER_INFO		1 << 1
#define DUMP_VERBOSE_INFO	1 << 2
#define DUMP_PACKET			1 << 3

static uint8_t packet_debug_switch;

static bool packet_dump(void* _data, size_t size, void* context) {
	if(unlikely(!!packet_debug_switch)) {
		printf("%s", context);
		if(packet_debug_switch & DUMP_INFO) {
			printf("Packet Lengh:\t%d\n", size);
		}

		Ether* eth = _data;
		if(packet_debug_switch & DUMP_ETHER_INFO) {
			printf("Ether Type:\t0x%04x\n", endian16(eth->type));
			uint64_t dmac = endian48(eth->dmac);
			uint64_t smac = endian48(eth->smac);
			printf("%02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
					(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
					(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
					(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
					(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
		}

		if(packet_debug_switch & DUMP_VERBOSE_INFO) {
			switch(eth->type) {
				case ETHER_TYPE_IPv4:
					break;
				case ETHER_TYPE_ARP:
					break;
				case ETHER_TYPE_IPv6:
					break;
				case ETHER_TYPE_LLDP:
					break;
				case ETHER_TYPE_8021Q:
					break;
				case ETHER_TYPE_8021AD:
					break;
				case ETHER_TYPE_QINQ1:
					break;
				case ETHER_TYPE_QINQ2:
					break;
				case ETHER_TYPE_QINQ3:
					break;
			}
		}

		if(packet_debug_switch & DUMP_PACKET) {
			uint8_t* data = (uint8_t*)_data;
			for(size_t i = 0 ; i < size;) {
				printf("\t0x%04x:\t", i);
				for(size_t j = 0; j < 16 && i < size; j++, i++) {
					printf("%02x ", data[i] & 0xff);
				}
				printf("\n");
			}
			printf("\n");
		}
	}

	return true;
}

static bool is_init;

static int cmd_dump(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(!is_init) {
		printf("Packet Dumper is not initialized\n");
		return 0;
	}

// FIXME: parse arguments
	packet_debug_switch = DUMP_INFO | DUMP_ETHER_INFO |
				DUMP_VERBOSE_INFO | DUMP_PACKET;

	return 0;
}

static Command commands[] = {
	{
		.name = "dump",
		.desc = "Packet dump",
		.func = cmd_dump
	}
};

int packet_dump_init() {
	if(nicdev_process_register(packet_dump, "RX Packet", NICDEV_RX_PROCESS)) goto error;

	if(nicdev_process_register(packet_dump, "TX Packet", NICDEV_TX_PROCESS)) goto error;

	if(nicdev_process_register(packet_dump, "SRX Packet", NICDEV_SRX_PROCESS)) goto error;

	if(nicdev_process_register(packet_dump, "STX Packet", NICDEV_STX_PROCESS)) goto error;

	if(cmd_register(commands, sizeof(commands) / sizeof(commands[0]))) goto error;

	is_init = true;

	return 0;

error:
	nicdev_process_unregister(NICDEV_RX_PROCESS);
	nicdev_process_unregister(NICDEV_TX_PROCESS);
	nicdev_process_unregister(NICDEV_SRX_PROCESS);
	nicdev_process_unregister(NICDEV_STX_PROCESS);

	return -1;
}