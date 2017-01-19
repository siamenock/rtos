#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <gmalloc.h>

#include <util/types.h>
#include <util/cmd.h>
#include <util/map.h>
#include <util/event.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/dhcp.h>

#include "stdio.h"
#include "cpu.h"
#include "timer.h"
#include "version.h"
#include "rtc.h"
#include "vnic.h"
#include "manager.h"
#include "device.h"
#include "port.h"
#include "acpi.h"
#include "vm.h"
#include "asm.h"
#include "file.h"
#include "pci.h"
#include "driver/charout.h"
#include "driver/charin.h"
#include "driver/disk.h"
#include "driver/nic.h"
#include "driver/fs.h"

#include "shell.h"


#define MAX_VM_COUNT	128
#define MAX_VNIC_COUNT	32

bool cmd_async;

#ifdef TEST
#include "test.h"
static int cmd_test(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	// Run all tests 
	if(argc == 1) {
		printf("Running PacketNgin RTOS all runtime tests...\n");
		if(run_test(NULL) < 0)
			return -1;

		return 0;
	}

	// List up test cases
	if(!strcmp("list", argv[1])) {
		list_tests();

		return 0;
	}

	// Run specific test case
	if(run_test(argv[1]) < 0)
		return -1;

	return 0;
}
#endif

static int cmd_clear(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	printf("\f");

	return 0;
}

static int cmd_echo(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	int pos = 0;
	for(int i = 1; i < argc; i++) {
		pos += sprintf(cmd_result + pos, "%s", argv[i]) - 1;
		if(i + 1 < argc) {
			cmd_result[pos++] = ' ';
		}
	}
	printf("%s\n", cmd_result);
	callback(cmd_result, 0);

	return 0;
}

static int cmd_sleep(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t time = 1;
	if(argc >= 2 && is_uint32(argv[1])) {
		time = parse_uint32(argv[1]);
	}
	timer_mwait(time);

	return 0;
}

static char* months[] = {
	"???",
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};

static char* weeks[] = {
	"???",
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
};

static int cmd_date(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t date = rtc_date();
	uint32_t time = rtc_time();

	printf("%s %s %d %02d:%02d:%02d UTC %d\n", weeks[RTC_WEEK(date)], months[RTC_MONTH(date)], RTC_DATE(date), 
			RTC_HOUR(time), RTC_MINUTE(time), RTC_SECOND(time), 2000 + RTC_YEAR(date));

	return 0;
}

static bool parse_addr(char* argv, uint32_t* address) {
	char* next = NULL;
	uint32_t temp;
	temp = strtol(argv, &next, 0);
	if(temp > 0xff)
		return false;

	*address = (temp & 0xff) << 24;
	if(next == argv)
		return false;
	argv = next;

	if(*argv != '.')
		return false;
	argv++;
	temp = strtol(argv, &next, 0);
	if(temp > 0xff)
		return false;

	*address |= (temp & 0xff) << 16;
	if(next == argv)
		return false;
	argv = next;

	if(*argv != '.')
		return false;
	argv++;
	temp = strtol(argv, &next, 0);
	if(temp > 0xff)
		return false;
	*address |= (temp & 0xff) << 8;
	if(next == argv)
		return false;
	argv = next;

	if(*argv != '.')
		return false;
	argv++;
	temp = strtol(argv, &next, 0);
	if(temp > 0xff)
		return false;
	*address |= temp & 0xff;
	if(next == argv)
		return false;
	argv = next;

	if(*argv != '\0')
		return false;

	return true;
}

static int cmd_manager(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(manager_nic == NULL) {
		printf("Manager not found\n");
		return -1;
	}

	if(argc == 1) {
		printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
				(manager_nic->mac >> 40) & 0xff,
				(manager_nic->mac >> 32) & 0xff,
				(manager_nic->mac >> 24) & 0xff,
				(manager_nic->mac >> 16) & 0xff,
				(manager_nic->mac >> 8) & 0xff,
				(manager_nic->mac >> 0) & 0xff);
		uint32_t ip = manager_get_ip();
		printf("%10sinet addr:%d.%d.%d.%d  ", "", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip >> 0) & 0xff);
		uint16_t port = manager_get_port();
		printf("port:%d\n", port);
		uint32_t mask = manager_get_netmask();
		printf("%10sMask:%d.%d.%d.%d\n", "", (mask >> 24) & 0xff, (mask >> 16) & 0xff, (mask >> 8) & 0xff, (mask >> 0) & 0xff);
		uint32_t gw = manager_get_gateway();
		printf("%10sGateway:%d.%d.%d.%d\n", "", (gw >> 24) & 0xff, (gw >> 16) & 0xff, (gw >> 8) & 0xff, (gw >> 0) & 0xff);

		return 0;
	}

	if(!manager_nic) {
		printf("Manager not found\n");
		return -1;
	}

	if(!strcmp("ip", argv[1])) {
		uint32_t old = manager_get_ip();
		if(argc == 2) {
			printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
			return 0;
		}

		uint32_t address;
		if(!parse_addr(argv[2], &address)) {
			printf("Address wrong\n");
			return 0;
		}

		manager_set_ip(address);
	} else if(!strcmp("port", argv[1])) {
		uint16_t old = manager_get_port();
		if(argc == 2) {
			printf("%d\n", old);
			return 0;
		}

		if(!is_uint16(argv[2])) {
			printf("Port number wrong\n");
			return -1;
		}

		uint16_t port = parse_uint16(argv[2]);

		manager_set_port(port);
	} else if(!strcmp("netmask", argv[1])) {
		uint32_t old = manager_get_netmask();
		if(argc == 2) {
			printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
			return 0;
		}

		uint32_t address;
		if(!parse_addr(argv[2], &address)) {
			printf("Address wrong\n");
			return 0;
		}

		manager_set_netmask(address);

		printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n",
				(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
				(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);

	} else if(!strcmp("gateway", argv[1])) {
		uint32_t old = manager_get_gateway();
		if(argc == 2) {
			printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
			return 0;
		}

		uint32_t address;
		if(!parse_addr(argv[2], &address)) {
			printf("Address wrong\n");
			return 0;
		}

		manager_set_gateway(address);

		printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n",
				(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
				(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	} else if(!strcmp("nic", argv[1])) {
		if(argc == 2) {
			printf("Network Interface name required\n");
			return false;
		}

		if(argc != 3) {
			printf("Wrong Parameter\n");
			return false;
		}

		uint16_t port = 0;
		Device* dev = nic_parse_index(argv[2], &port);
		if(!dev)
			return -2;

		uint64_t attrs[] = {
			NIC_MAC, ((NICPriv*)dev->priv)->mac[port >> 12],
			NIC_DEV, (uint64_t)argv[2],
			NIC_NONE
		};

		if(vnic_update(manager_nic, attrs)) {
			printf("Device not found\n");
			return -3;
		}
		manager_set_interface();

		return 0;
	} else
		return -1;

	return 0;
}

static int cmd_nic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	extern Device* nic_devices[];
	uint16_t nic_device_index = 0;


	if(argc == 1) {
		for(int i = 0; i < NIC_MAX_DEVICE_COUNT; i++) {
			Device* dev = nic_devices[i];
			if(!dev) {
				break;
			}

			NICPriv* nicpriv = dev->priv;
			MapIterator iter;
			map_iterator_init(&iter, nicpriv->nics);
			while(map_iterator_has_next(&iter)) {
				MapEntry* entry = map_iterator_next(&iter);
				uint16_t port = (uint16_t)(uint64_t)entry->key;
				uint16_t port_num = (port >> 12) & 0xf;
				uint16_t vlan_id = port & 0xfff;

				char name_buf[32];
				if(!vlan_id) {
					sprintf(name_buf, "eth%d", nic_device_index + port_num);
				} else {
					sprintf(name_buf, "eth%d.%d", nic_device_index + port_num, vlan_id);
				}
				printf("%12s", name_buf);
				printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
						(nicpriv->mac[port_num] >> 40) & 0xff,
						(nicpriv->mac[port_num] >> 32) & 0xff,
						(nicpriv->mac[port_num] >> 24) & 0xff,
						(nicpriv->mac[port_num] >> 16) & 0xff,
						(nicpriv->mac[port_num] >> 8) & 0xff,
						(nicpriv->mac[port_num] >> 0) & 0xff);
			}
			nic_device_index += nicpriv->port_count;
		}

		return 0;
	} else if(argc == 2) {
		for(int i = 0; i < NIC_MAX_DEVICE_COUNT; i++) {
			Device* dev = nic_devices[i];
			if(!dev) {
				break;
			}

			NICPriv* nicpriv = dev->priv;
			MapIterator iter;
			map_iterator_init(&iter, nicpriv->nics);
			while(map_iterator_has_next(&iter)) {
				MapEntry* entry = map_iterator_next(&iter);
				uint16_t port = (uint16_t)(uint64_t)entry->key;
				uint16_t port_num = (port >> 12) & 0xf;
				uint16_t vlan_id = port & 0xfff;

				char name_buf[32];
				if(!vlan_id) {
					sprintf(name_buf, "eth%d", nic_device_index + port_num);
				} else {
					sprintf(name_buf, "eth%d.%d", nic_device_index + port_num, vlan_id);
				}

				if(!strncmp(name_buf, argv[1], sizeof(argv[1]))) {
					printf("%12s", name_buf);
					printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
							(nicpriv->mac[port_num] >> 40) & 0xff,
							(nicpriv->mac[port_num] >> 32) & 0xff,
							(nicpriv->mac[port_num] >> 24) & 0xff,
							(nicpriv->mac[port_num] >> 16) & 0xff,
							(nicpriv->mac[port_num] >> 8) & 0xff,
							(nicpriv->mac[port_num] >> 0) & 0xff);
					return 0;
				} else
					continue;
			}
			nic_device_index += nicpriv->port_count;
		}
		printf("Device not found\n");

		return -3;

	} else {
		printf("Too many arguments\n");
		return -1;
	}
}

static int cmd_vnic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	extern Device* nic_devices[];
	uint16_t get_ni_index(Device* device) {
		uint16_t ni_index = 0;
		for(int i = 0; nic_devices[i] != NULL; i++) {
			if(device == nic_devices[i])
				return ni_index;
			else
				ni_index += ((NICPriv*)(nic_devices[i]->priv))->port_count;
		}

		return 0;
	}

	void print_byte_size(uint64_t byte_size) {
		uint64_t size = 1;
		for(int i = 0; i < 5; i++) {
			if((byte_size / size) < 1000) {
				printf("(%.1f ", (float)byte_size / size);
				switch(i) {
					case 0:
						printf("B)");
						break;
					case 1:
						printf("KB)");
						break;
					case 2:
						printf("MB)");
						break;
					case 3:
						printf("GB)");
						break;
					case 4:
						printf("TB)");
						break;
				}
				return;
			}

			size *= 1000;
		}
	}

	if(argc == 1 || (argc == 2 && !strcmp(argv[1], "list"))) {
		void print_vnic(VNIC* vnic, uint16_t vmid, uint16_t nic_index) {
			char name_buf[32];
			if(vmid)
				sprintf(name_buf, "veth%d.%d%c", vmid, nic_index, vnic == manager_nic ? '*' : ' ');
			else
				sprintf(name_buf, "veth%d%c", vmid, vnic == manager_nic ? '*' : ' ');

			printf("%12s", name_buf);
			printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x  ",
					(vnic->mac >> 40) & 0xff,
					(vnic->mac >> 32) & 0xff,
					(vnic->mac >> 24) & 0xff,
					(vnic->mac >> 16) & 0xff,
					(vnic->mac >> 8) & 0xff, 
					(vnic->mac >> 0) & 0xff);

			uint16_t port_num = vnic->port >> 12;
			uint16_t vlan_id = vnic->port & 0xfff;
			if(!vlan_id) {
				sprintf(name_buf, "eth%d", get_ni_index(vnic->device) + port_num);
			} else {
				sprintf(name_buf, "eth%d.%d\t", get_ni_index(vnic->device) + port_num, vlan_id);
			}
			printf("Parent %s\n", name_buf);
			printf("%12sRX packets:%d dropped:%d\n", "", vnic->nic->input_packets, vnic->nic->input_drop_packets);
			printf("%12sTX packets:%d dropped:%d\n", "", vnic->nic->output_packets, vnic->nic->output_drop_packets);
			printf("%12srxqueuelen:%d txqueuelen:%d\n", "", fifo_capacity(vnic->nic->input_buffer), fifo_capacity(vnic->nic->output_buffer));

			printf("%12sRX bytes:%lu ", "", vnic->nic->input_bytes);
			print_byte_size(vnic->nic->input_bytes);
			printf("  TX bytes:%lu ", vnic->nic->output_bytes);
			print_byte_size(vnic->nic->output_bytes);
			printf("\n");
			printf("%12sHead Padding:%d Tail Padding:%d", "",vnic->padding_head, vnic->padding_tail);
			printf("\n\n");
		}

		print_vnic(manager_nic, 0, 0);

		extern Map* vms;
		MapIterator iter;
		map_iterator_init(&iter, vms);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			uint16_t vmid = (uint16_t)(uint64_t)entry->key;
			VM* vm = entry->data;

			for(int i = 0; i < vm->nic_count; i++) {
				VNIC* vnic = vm->nics[i];
				print_vnic(vnic, vmid, i);
			}
		}
	} else
		return -1;

	return 0;
}

static int cmd_vlan(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	extern Device* nic_devices[];
	if(argc < 3) {
		printf("Wrong number of arguments\n");
		return -1;
	}

	if(!strcmp(argv[1], "add")) {
		if(argc != 4) {
			printf("Wrong number of arguments\n");
			return -1;
		}

		uint16_t port = 0;
		Device* dev = nic_parse_index(argv[2], &port);
		if(!dev) {
			printf("Device not found\n");
			return -2;
		}

		if(!is_uint16(argv[3])) {
			printf("VLAN ID wrong\n");
			return -3;
		}
		uint16_t vid = parse_uint16(argv[3]);

		if(vid == 0 || vid > 4096) {
			printf("VLAN ID wrong\n");
			return -3;
		}

		Map* nics = ((NICPriv*)dev->priv)->nics;
		port = (port & 0xf000) | vid;
		if(map_contains(nics, (void*)(uint64_t)port)) {
			printf("Already exist\n");
			return -3;
		}

		Map* vnics = map_create(8, NULL, NULL, NULL);
		if(!vnics) {
			printf("Cannot allocate vnic map\n");
			return -4;
		}
		if(!map_put(nics, (void*)(uint64_t)port, vnics)) {
			printf("Cannot add VLAN");
			map_destroy(vnics);
			return -4;
		}

		return 0;
	} else if(!strcmp(argv[1], "remove")) {
		if(argc != 3) {
			printf("Wrong number of arguments\n");
			return -1;
		}

		uint16_t port = 0;
		Device* dev = nic_parse_index(argv[2], &port);
		if(!dev)
			return -2;

		if(!(port & 0xfff)) { //vid == 0
			printf("VLan ID is 0\n");
			return -2;
		}

		Map* nics = ((NICPriv*)dev->priv)->nics;
		Map* vnics = map_get(nics, (void*)(uint64_t)port);
		if(!vnics) {
			printf("Cannot find VLAN\n");
			return -3;
		}

		if(!map_is_empty(vnics)) {
			printf("VLAN interface are busy\n");
			return -4;
		}

		if(!map_remove(nics, (void*)(uint64_t)port)) {
			printf("Cannot remove VLAN\n");
			return -5;
		}

		map_destroy(vnics);
	} else
		return -1;

	return 0;
}

static int cmd_version(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	printf("%d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG);

	return 0;
}

static int cmd_turbo(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc > 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	uint64_t perf_status = read_msr(0x00000198);
	perf_status = ((perf_status & 0xff00) >> 8);
	if(!cpu_has_feature(CPU_FEATURE_TURBO_BOOST)) {
		printf("Not Support Turbo Boost\n");
		printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);

		return 0;
	}

	uint64_t perf_ctrl = read_msr(0x00000199);

	if(argc == 1) {
		if(perf_ctrl & 0x100000000) {
			printf("Turbo Boost Disabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		} else {
			printf("Turbo Boost Enabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}

		return 0;
	}

	if(!strcmp(argv[1], "off")) {
		if(perf_ctrl & 0x100000000) {
			printf("Turbo Boost Already Disabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n",  perf_status / 10, perf_status % 10);
		} else {
			write_msr(0x00000199, 0x10000ff00);
			printf("Turbo Boost Disabled\n");
			perf_status = read_msr(0x00000198);
			perf_status = ((perf_status & 0xff00) >> 8);
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}
	} else if(!strcmp(argv[1], "on")) {
		if(perf_ctrl & 0x100000000) {
			write_msr(0x00000199, 0xff00);
			printf("Turbo Boost Enabled\n");
			perf_status = read_msr(0x00000198);
			perf_status = ((perf_status & 0xff00) >> 8);
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		} else {
			printf("Turbo Boost Already Enabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}
	} else {
		return -1;
	}

	return 0;
}

static int cmd_reboot(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	asm volatile("cli");

	uint8_t code;
	do {
		code = port_in8(0x64);	// Keyboard Control
		if(code & 0x01)
			port_in8(0x60);	// Keyboard I/O
	} while(code & 0x02);

	port_out8(0x64, 0xfe);	// Reset command

	while(1)
		asm("hlt");

	return 0;
}

static int cmd_shutdown(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	printf("Shutting down\n");
	acpi_shutdown();

	return 0;
}

static uint32_t arping_addr;
static uint32_t arping_count;
static uint64_t arping_time;
static uint64_t arping_event;

static bool arping_timeout(void* context) {
	if(arping_count <= 0)
		return false;

	arping_time = timer_ns();

	printf("Reply timeout\n");
	arping_count--;

	if(arping_count > 0) {
		return true;
	} else {
		printf("Done\n");
		return false;
	}
}

static int cmd_arping(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!parse_addr(argv[1], &arping_addr)) {
		printf("Address Wrong\n");
		return -1;
	}

	arping_time = timer_ns();

	arping_count = 1;
	if(argc >= 4) {
		if(strcmp(argv[2], "-c") == 0) {
			arping_count = strtol(argv[3], NULL, 0);
		}
	}

	if(arp_request(manager_nic->nic, arping_addr, 0)) {
		arping_event = event_timer_add(arping_timeout, NULL, 1000000, 1000000);
	} else {
		arping_count = 0;
		printf("Cannot send ARP packet\n");
	}

	return 0;
}

static int cmd_md5(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	if(!is_uint64(argv[2])) {
		return -2;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	uint64_t size = parse_uint64(argv[2]);
	uint32_t md5sum[4];
	bool ret = vm_storage_md5(vmid, size, md5sum);

	if(!ret) {
		sprintf(cmd_result, "(nil)");
		printf("Cannot md5 checksum\n");
	} else {
		char* p = (char*)cmd_result;
		for(int i = 0; i < 16; i++, p += 2) {
			sprintf(p, "%02x", ((uint8_t*)md5sum)[i]);
		}
		*p = '\0';
	}

	if(ret) {
		printf("%s\n", cmd_result);
		callback("true", 0);
	}
	return 0;
}

static int cmd_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	// Default value
	VMSpec* vm = malloc(sizeof(VMSpec));
	vm->core_size = 1;
	vm->memory_size = 0x1000000;		/* 16MB */
	vm->storage_size = 0x1000000;		/* 16MB */
	vm->nic_count = 0;
	vm->nics = malloc(sizeof(NICSpec) * MAX_VNIC_COUNT);
	vm->argc = 0;
	vm->argv = malloc(sizeof(char*) * CMD_MAX_ARGC);

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "core:") == 0) {
			i++;
			if(!is_uint8(argv[i])) {
				printf("Core must be uint8\n");
				return -1;
			}

			vm->core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "memory:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("Memory must be uint32\n");
				return -1;
			}

			vm->memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "storage:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("Storage must be uint32\n");
				return -1;
			}

			vm->storage_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "nic:") == 0) {
			i++;

			NICSpec* nic = &(vm->nics[vm->nic_count++]);
			nic->mac = 0;
			nic->dev = malloc(strlen("eth0") + 1);
			nic->dev = strcpy(nic->dev, "eth0");
			nic->input_buffer_size = 1024;
			nic->output_buffer_size = 1024;
			nic->input_bandwidth = 1000000000;	/* 1 GB */
			nic->output_bandwidth = 1000000000;	/* 1 GB */
			nic->pool_size = 0x400000;		/* 4 MB */

			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						if(!strcmp(argv[i], "dev_mac")) {
							nic->mac = NICSPEC_DEVICE_MAC;
							continue;
						} else {
							printf("Mac must be uint64\n");
							return i;
						}
					} else 
						nic->mac = parse_uint64(argv[i]) & 0xffffffffffff;
				} else if(strcmp(argv[i], "dev:") == 0) {
					i++;
					if(nic->dev)
						free(nic->dev);
					nic->dev = malloc(strlen(argv[i] + 1));
					strcpy(nic->dev, argv[i]);
				} else if(strcmp(argv[i], "ibuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("Ibuf must be uint32\n");
						return -1;
					}
					nic->input_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "obuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("Obuf must be uint32\n");
						return -1;
					}
					nic->output_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "iband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("Iband must be uint64\n");
						return -1;
					}
					nic->input_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "oband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("Oband must be uint64\n");
						return -1;
					}
					nic->output_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "hpad:") == 0) {
					i++;
					if(!is_uint16(argv[i])) {
						printf("Iband must be uint16\n");
						return -1;
					}
					nic->padding_head = parse_uint16(argv[i]);
				} else if(strcmp(argv[i], "tpad:") == 0) {
					i++;
					if(!is_uint16(argv[i])) {
						printf("Oband must be uint16\n");
						return -1;
					}
					nic->padding_tail = parse_uint16(argv[i]);
				} else if(strcmp(argv[i], "pool:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("Pool must be uint32\n");
						return -1;
					}
					nic->pool_size = parse_uint32(argv[i]);
				} else {
					i--;
					break;
				}
			}
		} else if(strcmp(argv[i], "args:") == 0) {
			i++;
			for( ; i < argc; i++) {
				vm->argv[vm->argc++] = argv[i];
			}
		}
	}

	if(vm->nic_count == 0) { 
		// Default value for NIC
		NICSpec* nic = &vm->nics[0]; 
		nic->mac = 0;
		nic->dev = malloc(strlen("eth0") + 1);
		nic->dev = strcpy(nic->dev, "eth0");
		nic->input_buffer_size = 1024;
		nic->output_buffer_size = 1024;
		nic->input_bandwidth = 1000000000;	/* 1 GB */
		nic->output_bandwidth = 1000000000;	/* 1 GB */
		nic->pool_size = 0x400000;		/* 4 MB */
		vm->nic_count = 1;
	}

	uint32_t vmid = vm_create(vm);
	if(vmid == 0) {
		callback(NULL, -1);
	} else {
		sprintf(cmd_result, "%d", vmid);
		printf("%s\n", cmd_result);
		callback(cmd_result, 0);
	}
	for(int i = 0; i < vm->nic_count; i++) {
		free(vm->nics[i].dev);
	}
	free(vm->argv);
	free(vm->nics);
	free(vm);
	return 0;
}

static int cmd_vm_destroy(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 1) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	bool ret = vm_destroy(vmid);

	if(ret) {
		printf("Vm destroy success\n");
		callback("true", 0);
	} else {
		printf("Vm destroy fail\n");
		callback("false", -1);
	}

	return 0;
}

static int cmd_vm_list(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t vmids[MAX_VM_COUNT];
	int len = vm_list(vmids, MAX_VM_COUNT);

	char* p = cmd_result;

	if(len <= 0) {
		printf("VM not found\n");
		callback("false", 0);
		return 0;
	}

	for(int i = 0; i < len; i++) {
		p += sprintf(p, "%lu", vmids[i]) - 1;
		if(i + 1 < len) {
			*p++ = ' ';
		} else {
			*p++ = '\0';
		}
	}

	printf("%s\n", cmd_result);
	callback("true", 0);
	return 0;
}

static int cmd_upload(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	int fd = open(argv[2], "r");
	if(fd < 0) {
		printf("Cannot open file: %s\n", argv[2]);
		return 1;
	}

	int offset = 0;
	int len;
	char buf[4096];
	while((len = read(fd, buf, 4096)) > 0) {
		if(vm_storage_write(vmid, buf, offset, len) != len) {
			printf("Upload fail : %s\n", argv[2]);
			callback("false", -1);
			return -1;
		}

		offset += len;
	}

	printf("Upload success : %d Total size : %d\n", vmid, offset);
	callback("true", 0);
	close(fd);
	return 0;
}

static void status_setted(bool result, void* context) {
	void(*callback)(char* result, int exit_status) = context;
	callback(result ? "true" : "false", 0);
}

static int cmd_status_set(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	if(!is_uint32(argv[1])) {
		callback("invalid", -1);
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	int status = 0;
	if(strcmp(argv[0], "start") == 0) {
		status = VM_STATUS_START;
	} else if(strcmp(argv[0], "pause") == 0) {
		status = VM_STATUS_PAUSE;
	} else if(strcmp(argv[0], "resume") == 0) {
		status = VM_STATUS_RESUME;
	} else if(strcmp(argv[0], "stop") == 0) {
		status = VM_STATUS_STOP;
	} else {
		printf("%s: command not found\n", argv[0]);
		callback("invalid", -1);
		return -1;
	}

	cmd_async = true;
	vm_status_set(vmid, status, status_setted, callback);

	return 0;
}

static int cmd_status_get(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	extern Map* vms;
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm) {
		printf("VM not found\n");
		return -1;
	}

	void print_vm_status(int status) {
		switch(status) {
			case VM_STATUS_START:
				printf("start");
				//callback("start", 0);
				break;
			case VM_STATUS_PAUSE:
				printf("pause");
				//callback("pause", 0);
				break;
			case VM_STATUS_STOP:
				printf("stop");
				//callback("stop", 0);
				break;
			default:
				printf("invalid");
				//callback("invalid", -1);
				break;
		}
	}

	printf("VM ID: %d\n", vmid);
	printf("Status: ");
	print_vm_status(vm->status);
	printf("\n");
	printf("Core size: %d\n", vm->core_size);
	printf("Core: ");
	for(int i = 0; i < vm->core_size; i++) {
		printf("[%d] ", vm->cores[i]);
	}
	printf("\n");

	return 0;
}


static int cmd_stdio(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		printf("Argument is not enough\n");
		return -1;
	}

	uint32_t id;
	uint8_t thread_id;

	if(!is_uint32(argv[1])) {
		printf("VM ID is wrong\n");
		return -1;
	}
	if(!is_uint8(argv[2])) {
		printf("Thread ID is wrong\n");
		return -2;
	}

	id = parse_uint32(argv[1]);
	thread_id = parse_uint8(argv[2]);

	for(int i = 3; i < argc; i++) {
		printf("%s\n", argv[i]);
		ssize_t len = vm_stdio(id, thread_id, 0, argv[i], strlen(argv[i]) + 1);
		if(!len) {
			printf("Stdio fail\n");
			return -i;
		}
	}

	return 0;
}

static int cmd_mount(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 5) {
		printf("Argument is not enough\n");
		return -1;
	}

	uint8_t type;
	uint8_t disk;
	uint8_t number;
	uint8_t partition;

	if(strcmp(argv[1], "-t") != 0) {
		printf("Argument is wrong\n");
		return -2;
	}

	if(strcmp(argv[2], "bfs") == 0 || strcmp(argv[2], "BFS") == 0) {
		type = FS_TYPE_BFS;
	} else if(strcmp(argv[2], "ext2") == 0 || strcmp(argv[2], "EXT2") == 0) {
		type = FS_TYPE_EXT2;
	} else if(strcmp(argv[2], "fat") == 0 || strcmp(argv[2], "FAT") == 0) {
		type = FS_TYPE_FAT;
	} else {
		printf("%s type is not supported\n", argv[2]);
		return -3;
	}

	if(argv[3][0] == 'v') {
		disk = DISK_TYPE_VIRTIO_BLK;
	} else if(argv[3][0] == 's') {
		disk = DISK_TYPE_USB;
	} else if(argv[3][0] == 'h') {
		disk = DISK_TYPE_PATA;
	} else {
		printf("%c type is not supported\n", argv[3][0]);
		return -3;
	}

	number = argv[3][2] - 'a';
	if(number > 8) {
		printf("Disk number cannot exceed %d\n", DISK_AVAIL_DEVICES);
		return -4;
	}

	partition = argv[3][3] - '1';
	if(partition > 3) {
		printf("Partition number cannot exceed 4\n");
		return -5;
	}

	fs_mount(disk << 16 | number, partition, type, argv[4]);

	return -2;
}

static int cmd_dhcp(int argc, char** argv, void(*callback)(char* result, int exit_status)) {

	bool manager_ip_acked(NIC* nic, uint32_t transaction_id, uint32_t ip, void* _data) {
		printf("Manager ip leased \n");
		uint32_t manager_ip = manager_set_ip(ip);
		uint32_t gw = (ip & 0xffffff00) | 0x1;
		manager_set_gateway(gw);
		printf("%10sinet addr:%d.%d.%d.%d  ", "", (manager_ip >> 24) & 0xff, (manager_ip >> 16) & 0xff, (manager_ip >> 8) & 0xff, (manager_ip >> 0) & 0xff);
		return true;
	}

	if(dhcp_lease_ip(manager_nic->nic, NULL, manager_ip_acked, NULL) == 0)
		printf("Failed to lease Manager IP : %d\n", errno);

	return 0;
}

/*
 * Basic display modes:
 -mm		Produce machine-readable output (single -m for an obsolete format)
 -t		Show bus tree

 * Display options:
 -v		Be verbose (-vv for very verbose)
 -k		Show kernel drivers handling each device
 -x		Show hex-dump of the standard part of the config space
 -xxx		Show hex-dump of the whole config space (dangerous; root only)
 -xxxx		Show hex-dump of the 4096-byte extended config space (root only)
 -b		Bus-centric view (addresses and IRQ's as seen by the bus)
 -D		Always show domain numbers

 * Resolving of device ID's to names:
 -n		Show numeric ID's
 -nn		Show both textual and numeric ID's (names & numbers)
 -q		Query the PCI ID database for unknown ID's via DNS
 -qq		As above, but re-query locally cached entries
 -Q		Query the PCI ID database for all ID's via DNS

 * Selection of devices:
 -s [[[[<domain>]:]<bus>]:][<slot>][.[<func>]]	Show only devices in selected slots
 -d [<vendor>]:[<device>]			Show only devices with specified ID's

 * Other options:
 -i <file>	Use specified ID database instead of /usr/share/misc/pci.ids.gz
 -p <file>	Look up kernel modules in a given file instead of default modules.pcimap
 -M		Enable `bus mapping' mode (dangerous; root only)

 * PCI access options:
 -A <method>	Use the specified PCI access method (see `-A help' for a list)
 -O <par>=<val>	Set PCI access parameter (see `-O help' for a list)
 -G		Enable PCI access debugging
 -H <mode>	Use direct hardware access (<mode> = 1 or 2)
 -F <file>	Read PCI configuration dump from a given file
 */
static int cmd_lspci(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	/* Basic display modes:*/
	bool m = false;
	bool t = false;

	/* Display options: */
	bool v = false;		// show verbose
	bool k = false;		// show drivers
	uint8_t x = 0;	// hex-dump
	bool b = false;		// kbus-centric view
	bool D = false;	// show domain

	/* Resolving of device ID's to names: */
	uint8_t n = 0;	// show numeric id's
	// 	uint8_t q;	// query the pci id databse
	uint16_t s_bus = 0xffff;	// selected slots.
	uint8_t s_slot = 0xff;
	uint8_t s_function = 0xff;

	bool d = false;	// selected ids.

	for(int i = 1; i < argc; i++) {
		if(*argv[i] == '-') {
			for(int j = 1; j < strlen(argv[i]); j++) {
				if(!strncmp(&argv[i][j], "mm", 1)) {
					m = true;
				} else if(!strncmp(&argv[i][j], "t", 1)) {
					t = true;
				} else if(!strncmp(&argv[i][j], "v", 1)) {
					v = true;
				} else if(!strncmp(&argv[i][j], "k", 1)) {
					k = true;
				} else if(!strncmp(&argv[i][j], "x", 1)) {
					if(!strncmp(&argv[i][j], "xxxx", 4)) {
						x = PCI_DUMP_LEVEL_XXXX;
						j+= 3;
					} else if(!strncmp(&argv[i][j], "xxx", 3)) {
						x = PCI_DUMP_LEVEL_XXX;
						x = 3;
						j+= 2;
					} else if(!strncmp(&argv[i][j], "xx", 2)) {
						x = PCI_DUMP_LEVEL_XX;
						j+= 1;
					} else {// "x"
						x = PCI_DUMP_LEVEL_X;
					}
				} else if(!strncmp(&argv[i][j], "b", 1)) {
					b = true;
				} else if(!strncmp(&argv[i][j], "D", 1)) {
					D = true;
				} else if(!strncmp(&argv[i][j], "n", 1)) {
					if(!strncmp(&argv[i][j], "nn", 2)) {
						n = 2;
						j++;
					} else {
						n = 1;
					}
				} else if(!strncmp(&argv[i][j], "s", 1)) {
					i++;
					char* next;
					uint16_t ret = strtol(argv[i], &next, 16);
					if(*next == ':') {
						s_bus = ret;
						next++;
						ret = strtol(next, &next, 16);
					}
					s_slot = ret;

					if(*next == '.') {
						next++;
						s_function = strtol(next, NULL, 16);
					}

					break;
				} else if(!strncmp(&argv[i][j], "d", 1)) {
					i++;

					break;
				} else {
					//TODO print lspci help
					return -2;
				}
			}
		}
	}

	if(t) {
		PCI_Bus_Entry* bus_entrys = gmalloc(sizeof(PCI_Bus_Entry) * PCI_MAX_BUS);
		memset(bus_entrys, 0, sizeof(PCI_Bus_Entry) * PCI_MAX_BUS);
		uint8_t bus_count = pci_get_entrys(bus_entrys);

		for(uint32_t bus_index = 0; bus_index < bus_count; bus_index++) {
			PCI_Bus_Entry* bus_entry = &bus_entrys[bus_index];
			printf("-[%04x:%02x]-", 0, bus_entry->bus); // domain:bus
			for(uint32_t slot_index = 0; slot_index < bus_entry->slot_count; slot_index++) {
				PCI_Slot_Entry* slot_entry = &bus_entry->slot_entry[slot_index];
				for(uint32_t function_index = 0; function_index < slot_entry->function_count; function_index++) {
					PCI_Function_Entry* function_entry = &slot_entry->function_entry[function_index];
					if(!(function_index == 0 && slot_index == 0))
						printf("           ");

					if(function_index == (slot_entry->function_count - 1) && slot_index == (bus_entry->slot_count - 1)) 
						printf("\\"); // Last function entry
					else
						printf("+");

					printf("-%02x.%x", slot_entry->slot, function_entry->function);

					//TODO Brdige
					// 				if() {
					// 					printf("-[%02x]--");
					// 					if(0)
					// 						printf("--%02x.%x");
					// 				}
					printf("\n");
				}
			}
		}

		gfree(bus_entrys);
	} else {
		PCI_Bus_Entry* bus_entrys = gmalloc(sizeof(PCI_Bus_Entry) * PCI_MAX_BUS);
		memset(bus_entrys, 0, sizeof(PCI_Bus_Entry) * PCI_MAX_BUS);
		uint8_t bus_count = pci_get_entrys(bus_entrys);

		for(uint32_t bus_index = 0; bus_index < bus_count; bus_index++) {
			PCI_Bus_Entry* bus_entry = &bus_entrys[bus_index];
			if(s_bus != 0xffff) {
				if(bus_entry->bus != s_bus)
					continue;
			}
			for(uint32_t slot_index = 0; slot_index < bus_entry->slot_count; slot_index++) {
				PCI_Slot_Entry* slot_entry = &bus_entry->slot_entry[slot_index];
				if(s_slot != 0xff) {
					if(slot_entry->slot != s_slot)
						continue;
				}
				for(uint32_t function_index = 0; function_index < slot_entry->function_count; function_index++) {
					PCI_Function_Entry* function_entry = &slot_entry->function_entry[function_index];
					if(s_function != 0xff) {
						if(function_entry->function != s_function)
							continue;
					}
					//TODO Query to DNS PCI information
					printf("%02x:%02x.%x\n", bus_entry->bus, slot_entry->slot, function_entry->function);
					if(x)
						pci_data_dump(bus_entry->bus, slot_entry->slot, function_entry->function, x);
				}
			}
		}
		gfree(bus_entrys);
	}

	return 0;
}

Command commands[] = {
	{
		.name = "help",
		.desc = "Show this message.", 
		.func = cmd_help 
	},
#ifdef TEST
	{
		.name = "test",
		.desc = "Run run-time tests.", 
		.func = cmd_test
	},
#endif
	{ 
		.name = "version", 
		.desc = "Print the kernel version.", 
		.func = cmd_version 
	},
	{
		.name = "turbo",
		.desc = "Turbo Boost Enable/Disable",
		.args = "[on/off]",
		.func = cmd_turbo
	},
	{ 
		.name = "clear", 
		.desc = "Clear screen.", 
		.func = cmd_clear 
	},
	{ 
		.name = "echo", 
		.desc = "Echo arguments.",
		.args = "[variable: string]*",
		.func = cmd_echo 
	},
	{
		.name = "sleep",
		.desc = "Sleep n seconds",
		.args = "[n: uint32]",
		.func = cmd_sleep
	},
	{ 
		.name = "date", 
		.desc = "Print current date and time.", 
		.func = cmd_date 
	},
	{ 
		.name = "manager",
		.desc = "Set ip, port, netmask, gateway, nic of manager",
		.func = cmd_manager
	},
	{ 
		.name = "nic",
		.desc = "List, up, down, manager of network interface ",
		.func = cmd_nic
	},
	{ 
		.name = "vnic",
		.desc = "List of virtual network interface",
		.func = cmd_vnic
	},
	{
		.name = "vlan",
		.desc = "Add or remove vlan",
		.args = "[device name][vid]",
		.func = cmd_vlan
	},
	{ 
		.name = "reboot",
		.desc = "Reboot the node.",
		.func = cmd_reboot
	},
	{ 
		.name = "shutdown", 
		.desc = "Shutdown the node.", 
		.func = cmd_shutdown 
	},
	{ 
		.name = "halt", 
		.desc = "Shutdown the node.", 
		.func = cmd_shutdown 
	},
	{ 
		.name = "arping", 
		.desc = "ARP ping to the host.",
		.args = "(address) [\"-c\" (count)]",
		.func = cmd_arping 
	},
	{ 
		.name = "create", 
		.desc = "Create VM",
		.args = "vmid: uint32, core: (number: int) memory: (size: uint32) storage: (size: uint32) [nic: mac: (addr: uint64) ibuf: (size: uint32) obuf: (size: uint32) iband: (size: uint64) oband: (size: uint64) pool: (size: uint32)]* [args: [string]+ ]",
		.func = cmd_create 
	},
	{
		.name = "destroy",
		.desc = "Destroy VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_vm_destroy
	},
	{
		.name = "list",
		.desc = "List VM",
		.args = "result: uint64[]",
		.func = cmd_vm_list
	},
	{
		.name = "upload",
		.desc = "Upload file",
		.args = "result: bool, vmid: uint32 path: string",
		.func = cmd_upload
	},
	{
		.name = "md5",
		.desc = "MD5 storage",
		.args = "result: hex16 string, vmid: uint32 size: uint64",
		.func = cmd_md5
	},
	{ 
		.name = "start", 
		.desc = "Start VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set 
	},
	{
		.name = "pause",
		.desc = "Pause VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "resume",
		.desc = "Resume VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "stop",
		.desc = "Stop VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "status",
		.desc = "Get VM's status",
		.args = "result: string(\"start\", \"pause\", or \"stop\") vmid: uint32",
		.func = cmd_status_get
	},
	{
		.name = "stdin",
		.desc = "Write stdin to vm",
		.args = "result: bool, vmid: uint32 thread_id: uint8 msg: string",
		.func = cmd_stdio
	},
/*
	{
		.name = "mount",
		.desc = "Mount file system",
		.args = "result: bool, [\"-t\" (type)] device: string dir: string",
		.func = cmd_mount
	},
*/
	{
		.name = "dhcp",
		.desc = "DHCP get manager ip",
		.args = "result: bool",
		.func = cmd_dhcp
	},
	/*
	 *{
	 *        .name = "lspci",
	 *        .desc = "list all PCI Devices",
	 *        .args = "-t\tShow a tree of bus\n\
	 *                 -x\tShow hexadeciaml dump of PCI configuration data. (64Bytes)\n\
	 *                 -xxx\tShow hexademical dump of PCI configuration data. (256Bytes)\n\
	 *                 -xxxx\tShow hexademical dump of PCI configuration data. (4096Bytes)\n\
	 *                 -s\tSelect Device [<bus>:][<slot>][.<func>]\n",
	 *        .func = cmd_lspci
	 *},
	 */
	{
		.name = NULL
	},
};

static void cmd_callback(char* result, int exit_status) {
	if(!result)
		return;
	cmd_update_var(result, exit_status);
	cmd_async = false;
}

static void history_erase() {
	if(!cmd_history.using())
		// Nothing to be erased
		return;

	/*
	 *if(cmd_history.index >= cmd_history.count() - 1)
	 *        // No more entity to be erased
	 *        return;
	 *        
	 */
	int len = strlen(cmd_history.get_current());
	for(int i = 0; i < len; i++)
		putchar('\b');
}

void shell_callback() {
	static char cmd[CMD_SIZE];
	static int cmd_idx = 0;
	extern Device* device_stdout;
	char* line;

	if(cmd_async)
		return;

	int ch = stdio_getchar();
	while(ch >= 0) {
		switch(ch) {
			case '\n':
				cmd[cmd_idx] = '\0';
				putchar(ch);
				if(!cmd_history.using())
					cmd_history.save(cmd);

				int exit_status = cmd_exec(cmd, cmd_callback);
				if(exit_status != 0) { 
					if(exit_status == CMD_STATUS_WRONG_NUMBER) {
						printf("Wrong number of arguments\n");
					} else if(exit_status == CMD_STATUS_NOT_FOUND) {
						printf("Command not found: %s\n", cmd);
					} else if(exit_status == CMD_VARIABLE_NOT_FOUND) {
						printf("Variable not found\n");
					} else if(exit_status < 0) { 
						printf("Wrong value of argument: %d\n", -exit_status);
					}
				}

				printf("# ");
				cmd_idx = 0;
				cmd_history.reset();
				break;
			case '\f':
				putchar(ch);
				break;
			case '\b':
				if(cmd_idx > 0) {
					cmd_idx--;
					putchar(ch);
				}
				break;
			case 0x1b:	// ESC
				ch = stdio_getchar();
				switch(ch) {
					case '[':
						ch = stdio_getchar();
						switch(ch) {
							case 0x35:
								stdio_getchar();	// drop '~'
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, -12);
								break;
							case 0x36:
								stdio_getchar();	// drop '~'
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, 12);
								break;
						}
						break;
					case 'O':
						ch = stdio_getchar();
						switch(ch) {
							case 'A': // Up
								history_erase();
								line = cmd_history.get_past();
								if(line) {
									printf("%s", line);
									strcpy(cmd, line);
									cmd_idx = strlen(line);
								}
								break;
							case 'B': // Down
								history_erase();
								line = cmd_history.get_later();
								if(line) {
									printf("%s", line);
									strcpy(cmd, line);
									cmd_idx = strlen(line);
								} else {
									cmd_history.reset();
									cmd_idx = 0;
								}
								break;
							case 'H':
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, INT32_MIN);
								break;
							case 'F':
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, INT32_MAX);
								break;
						}
						break;
				}
				break;
			default:
				if(cmd_idx < CMD_SIZE - 1) {
					cmd[cmd_idx++] = ch;
					putchar(ch);
				}
		}
		if(cmd_async)
			break;

		ch = stdio_getchar();
	}
}

void shell_init() {
	printf("\nPacketNgin ver %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG);
	printf("# ");

	extern Device* device_stdin;
	((CharIn*)device_stdin->driver)->set_callback(device_stdin->id, shell_callback);
	cmd_async = false;
	cmd_init();
}

bool shell_process(Packet* packet) {
	if(arping_count == 0)
		return false;

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_ARP)
		return false;

	ARP* arp = (ARP*)ether->payload;
	switch(endian16(arp->operation)) {
		case 2: // Reply
			;
			uint64_t smac = endian48(arp->sha);
			uint32_t sip = endian32(arp->spa);

			if(arping_addr == sip) {
				uint32_t time = timer_ns() - arping_time;
				uint32_t ms = time / 1000;
				uint32_t ns = time - ms * 1000; 

				printf("Reply from %d.%d.%d.%d [%02x:%02x:%02x:%02x:%02x:%02x] %d.%dms\n",
						(sip >> 24) & 0xff,
						(sip >> 16) & 0xff,
						(sip >> 8) & 0xff,
						(sip >> 0) & 0xff,
						(smac >> 40) & 0xff,
						(smac >> 32) & 0xff,
						(smac >> 24) & 0xff,
						(smac >> 16) & 0xff,
						(smac >> 8) & 0xff,
						(smac >> 0) & 0xff,
						ms, ns);

				event_timer_remove(arping_event);
				arping_count--;

				if(arping_count > 0) {
					bool arping(void* context) {
						arping_time = timer_ns();
						if(arp_request(manager_nic->nic, arping_addr, 0)) {
							arping_event = event_timer_add(arping_timeout, NULL, 1000000, 1000000);
						} else {
							arping_count = 0;
							printf("Cannot send ARP packet\n");
						}

						return false;
					}

					event_timer_add(arping, NULL, 1000000, 1000000);
				} else {
					printf("Done\n");
				}
			}
			break;
	}

	return false;
}
