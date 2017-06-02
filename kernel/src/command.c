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
#include <net/interface.h>
#include <net/ether.h>
#include <net/arp.h>
#include <vnic.h>

#include "stdio.h"
#include "cpu.h"
#include "timer.h"
#include "version.h"
#include "rtc.h"
#include "manager.h"
#include "device.h"
#include "port.h"
#include "acpi.h"
#include "vm.h"
#include "asm.h"
#include "msr.h"
#include "file.h"
#include "driver/disk.h"
#include "driver/nicdev.h"
#include "driver/fs.h"

#include "shell.h"

#define MAX_VM_COUNT	128
#define NEXT_ARGUMENTS()		i++; \
					if(!(i < argc)) \
						break; \
					else if(!strncmp(argv[i], "-", 1)) { \
						i--; \
						continue; \
					}
		

static void print_byte_size(uint64_t byte_size) {
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

static void print_vnic_metadata(VNIC* vnic) {
	printf("%12sRX packets:%d dropped:%d\n", "", vnic->input_packets, vnic->input_drop_packets);
	printf("%12sTX packets:%d dropped:%d\n", "", vnic->output_packets, vnic->output_drop_packets);

	printf("%12sRX bytes:%lu ", "", vnic->input_bytes);
	print_byte_size(vnic->input_bytes);
	printf("  TX bytes:%lu ", vnic->output_bytes);
	print_byte_size(vnic->output_bytes);
	printf("\n");
	printf("%12sHead Padding:%d Tail Padding:%d\n", "",vnic->padding_head, vnic->padding_tail);
}

static void print_vnic(VNIC* vnic, uint16_t vmid, uint16_t nic_index) {
	char name_buf[32];
	sprintf(name_buf, "v%deth%d", vmid, nic_index);
	printf("%12s", name_buf);
	printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x  ",
			(vnic->mac >> 40) & 0xff,
			(vnic->mac >> 32) & 0xff,
			(vnic->mac >> 24) & 0xff,
			(vnic->mac >> 16) & 0xff,
			(vnic->mac >> 8) & 0xff,
			(vnic->mac >> 0) & 0xff);

	printf("Parent %s\n", vnic->parent);
	print_vnic_metadata(vnic);
	printf("\n");
}

static void print_interface(VNIC* vnic, uint16_t vmid, uint16_t nic_index) {
	IPv4InterfaceTable* table = interface_table_get(vnic->nic);
	if(!table)
		return;

	int offset = 1;
	for(int interface_index = 0; interface_index < IPV4_INTERFACE_MAX_COUNT; interface_index++, offset <<= 1) {
		IPv4Interface* interface = NULL;
		if(table->bitmap & offset)
			 interface = &table->interfaces[interface_index];
		else if(interface_index)
			continue;

		char name_buf[32];
		if(interface_index)
			sprintf(name_buf, "v%deth%d:%d", vmid, nic_index, interface_index - 1);
		else
			sprintf(name_buf, "v%deth%d", vmid, nic_index);

		printf("%12s", name_buf);
		printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x  ",
				(vnic->mac >> 40) & 0xff,
				(vnic->mac >> 32) & 0xff,
				(vnic->mac >> 24) & 0xff,
				(vnic->mac >> 16) & 0xff,
				(vnic->mac >> 8) & 0xff,
				(vnic->mac >> 0) & 0xff);
		printf("Parent %s\n", vnic->parent);

		if(interface) {
			uint32_t ip = interface->address;
			printf("%12sinet addr:%d.%d.%d.%d  ", "", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip >> 0) & 0xff);
			uint32_t mask = interface->netmask;
			printf("\tMask:%d.%d.%d.%d\n", (mask >> 24) & 0xff, (mask >> 16) & 0xff, (mask >> 8) & 0xff, (mask >> 0) & 0xff);
			//uint32_t gw = interface->gateway;
			//printf("%12sGateway:%d.%d.%d.%d\n", "", (gw >> 24) & 0xff, (gw >> 16) & 0xff, (gw >> 8) & 0xff, (gw >> 0) & 0xff);
		}

		if(interface_index == 0)
			print_vnic_metadata(vnic);
		printf("\n");
	}
}

static bool parse_vnic_interface(char* name, uint16_t* vmid, uint16_t* vnic_index, uint16_t* interface_index) {
	if(strncmp(name, "v", 1))
		return false;

	char* next;
	char* _vmid = strtok_r(name + 1, "eth", &next);
	if(!_vmid)
		return false;

	if(!is_uint8(_vmid))
		return false;

	*vmid = parse_uint8(_vmid);

	char* _interface_index;
	char* _vnic_index = strtok_r(next, ":", &_interface_index);
	if(!_vnic_index)
		return false;

	if(!is_uint8(_vnic_index))
		return false;

	*vnic_index = parse_uint8(_vnic_index);

	if(_interface_index) {
		if(!is_uint8(_interface_index))
			return false;

		*interface_index = parse_uint8(_interface_index) + 1;
	} else
		*interface_index = 0;

	return true;
}

// 
// #ifdef TEST
// #include "test.h"
// static int cmd_test(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
// 	// Run all tests
// 	if(argc == 1) {
// 		printf("Running PacketNgin RTOS all runtime tests...\n");
// 		if(run_test(NULL) < 0)
// 			return -1;
// 
// 		return 0;
// 	}
// 
// 	// List up test cases
// 	if(!strcmp("list", argv[1])) {
// 		list_tests();
// 
// 		return 0;
// 	}
// 
// 	// Run specific test case
// 	if(run_test(argv[1]) < 0)
// 		return -1;
// 
// 	return 0;
// }
// #endif

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

//TODO
//manager destroy vnic veth0
//manager show vnic
//manager set veth0.1 
static int cmd_manager(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	//TODO error handle
	for(int i = 1; i < argc; i++) {
		if(!strcmp("create", argv[i])) {
			NEXT_ARGUMENTS();

			VNIC* vnic = manager_create_vnic(argv[i]);
			if(!vnic)
				printf("fail\n");

			return 0;
		} else if(!strcmp("destroy", argv[i])) {
			NEXT_ARGUMENTS();

			uint16_t vmid;
			uint16_t vnic_index;
			uint16_t interface_index;
			if(!parse_vnic_interface(argv[i], &vmid, &vnic_index, &interface_index))
				return -i;

			if(vmid)
				return -i;

			VNIC* vnic = manager.vnics[vnic_index];
			if(!vnic)
				return -i;

			if(!manager_destroy_vnic(vnic))
				printf("fail\n");

			return 0;
		} else if(!strcmp("show", argv[i])) {
			for(int i = 0; i < NIC_MAX_COUNT; i++) {
				VNIC* vnic = manager.vnics[i];	// gmalloc, ni_create
				if(!vnic)
					continue;

				print_interface(vnic, 0, i);
			}

			return 0;
		} else if(!strcmp("open", argv[i])) {
			NEXT_ARGUMENTS();

			uint16_t vmid;
			uint16_t vnic_index;
			uint16_t interface_index;
			if(!parse_vnic_interface(argv[i], &vmid, &vnic_index, &interface_index))
				return -i;

			if(vmid)
				return -i;

			VNIC* vnic = manager.vnics[vnic_index];
			if(!vnic)
				return -i;

			IPv4InterfaceTable* table = interface_table_get(vnic->nic);
			if(!table)
				return -i;

			uint16_t offset = 1;
			for(int i = 0; i < interface_index; i++)
				offset <<= 1;

			if(!(table->bitmap & offset))
				return -i;

			IPv4Interface* interface = &table->interfaces[interface_index];
			struct netif* netif = manager_create_netif(vnic->nic, interface->address, interface->netmask, interface->gateway, false);
			printf("open: %x %x %x\n", interface->address, interface->netmask, interface->gateway);
			manager_netif_server_open(netif, 1111);

			return 0;
		} else if(!strcmp("close", argv[i])) {
			return 0;
		}
	}
	return 0;
}
	// 
	// 	if(!strcmp("ip", argv[1])) {
	// 		uint32_t old = manager_get_ip();
	// 		if(argc == 2) {
	// 			printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
	// 			return 0;
	// 		}
	// 
	// 		uint32_t address;
	// 		if(!parse_addr(argv[2], &address)) {
	// 			printf("Address wrong\n");
	// 			return 0;
	// 		}
	// 
	// 		manager_set_ip(address);
	// 	} else if(!strcmp("port", argv[1])) {
	// 		uint16_t old = manager_get_port();
	// 		if(argc == 2) {
	// 			printf("%d\n", old);
	// 			return 0;
	// 		}
	// 
	// 		if(!is_uint16(argv[2])) {
	// 			printf("Port number wrong\n");
	// 			return -1;
	// 		}
	// 
	// 		uint16_t port = parse_uint16(argv[2]);
	// 
	// 		manager_set_port(port);
	// 	} else if(!strcmp("netmask", argv[1])) {
	// 		uint32_t old = manager_get_netmask();
	// 		if(argc == 2) {
	// 			printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
	// 			return 0;
	// 		}
	// 
	// 		uint32_t address;
	// 		if(!parse_addr(argv[2], &address)) {
	// 			printf("Address wrong\n");
	// 			return 0;
	// 		}
	// 
	// 		manager_set_netmask(address);
	// 
	// 		printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n",
	// 			(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
	// 			(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	// 
	// 	} else if(!strcmp("gateway", argv[1])) {
	// 		uint32_t old = manager_get_gateway();
	// 		if(argc == 2) {
	// 			printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
	// 			return 0;
	// 		}
	// 
	// 		uint32_t address;
	// 		if(!parse_addr(argv[2], &address)) {
	// 			printf("Address wrong\n");
	// 			return 0;
	// 		}
	// 
	// 		manager_set_gateway(address);
	// 
	// 		printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n",
	// 			(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
	// 			(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	// 	} else if(!strcmp("nic", argv[1])) {
	// 		if(argc == 2) {
	// 			printf("Network Interface name required\n");
	// 			return false;
	// 		}
	// 
	// 		if(argc != 3) {
	// 			printf("Wrong Parameter\n");
	// 			return false;
	// 		}
	// 
	// 		uint16_t port = 0;
	// 		Device* dev = nicdev_get(argv[2]);
	// 		if(!dev)
	// 			return -2;
	// 
	// 		uint64_t attrs[] = {
	// 			VNIC_MAC, ((NICDevice*)dev->priv)->mac,
	// 			VNIC_DEV, (uint64_t)argv[2],
	// 			VNIC_NONE
	// 		};
	// 
	// 		/*
	// 		 *if(vnic_update(manager_nic, attrs)) {
	// 		 *        printf("Device not found\n");
	// 		 *        return -3;
	// 		 *}
	// 		 */
	// 		manager_set_interface();
	// 
	// 		return 0;
	// 	} else
	// 		return -1;
	// 
	// 	return 0;

static int cmd_nic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	int nicdev_count = nicdev_get_count();
	for(int i = 0 ; i < nicdev_count; i++) {
		NICDevice* nicdev = nicdev_get_by_idx(i);
		if(!nicdev)
			break;

		printf("%12s", nicdev->name);
		printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
				(nicdev->mac >> 40) & 0xff,
				(nicdev->mac >> 32) & 0xff,
				(nicdev->mac >> 24) & 0xff,
				(nicdev->mac >> 16) & 0xff,
				(nicdev->mac >> 8) & 0xff,
				(nicdev->mac >> 0) & 0xff);
	}

	return 0;
}

static int cmd_vnic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc == 1) {
		for(int i = 0; i < NIC_MAX_COUNT; i++) {
			VNIC* vnic = manager.vnics[i];	// gmalloc, ni_create
			if(!vnic)
				continue;

			print_vnic(vnic, 0, i);
		}

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
			printf("\n");
		}

		return 0;
	}

	return 0;
}

static int cmd_interface(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc == 1) {
		for(int i = 0; i < NIC_MAX_COUNT; i++) {
			VNIC* vnic = manager.vnics[i];	// gmalloc, ni_create
			if(!vnic)
				continue;

			print_interface(vnic, 0, i);
		}

		extern Map* vms;
		MapIterator iter;
		map_iterator_init(&iter, vms);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			uint16_t vmid = (uint16_t)(uint64_t)entry->key;
			VM* vm = entry->data;

			for(int i = 0; i < vm->nic_count; i++) {
				VNIC* vnic = vm->nics[i];
				print_interface(vnic, vmid, i);
			}
		}

		return 0;
	} else {
		uint16_t vmid;
		uint16_t vnic_index;
		uint16_t interface_index;
		if(!parse_vnic_interface(argv[1], &vmid, &vnic_index, &interface_index))
			return -1;

		VNIC* vnic;
		if(vmid) { //Virtual Machine VNIC
			extern Map* vms;
			VM* vm = map_get(vms, (void*)(uint64_t)vmid);
			if(!vm)
				return -2;

			if(vnic_index > vm->nic_count)
				return -2;

			vnic = vm->nics[vnic_index];
		} else { //Manager VNIC
			vnic = manager.vnics[vnic_index];
		}

		if(!vnic)
			return -2;

		IPv4InterfaceTable* table = interface_table_get(vnic->nic);
		if(!table)
			return -3;

		IPv4Interface* interface = NULL;
		uint16_t offset = 1;
		for(int i = 0; i < interface_index; i++)
			offset <<= 1;

		table->bitmap |= offset;
		interface = &table->interfaces[interface_index];

		if(argc > 2) {
			uint32_t address;
			if(is_uint32(argv[2])) {
				address = parse_uint32(argv[2]);
			} else if(!parse_addr(argv[2], &address)) {
				printf("Address wrong\n");
				return 0;
			}
			if(address == 0) { //disable
				table->bitmap ^= offset;
				return 0;
			}

			interface->address = address;
		}
		if(argc > 3) {
			uint32_t netmask;
			if(is_uint32(argv[3])) {
				netmask = parse_uint32(argv[3]);
			} else if(!parse_addr(argv[3], &netmask)) {
				printf("Address wrong\n");
				return 0;
			}
			interface->netmask = netmask;
		}
		if(argc > 4) {
			uint32_t gateway;
			if(is_uint32(argv[4])) {
				gateway = parse_uint32(argv[4]);
			} else if(!parse_addr(argv[4], &gateway)) {
				printf("Address wrong\n");
				return 0;
			}
			interface->gateway = gateway;
		}
	}

	return 0;
}

static int cmd_vlan(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		printf("Wrong number of arguments\n");
		return -1;
	}

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "add")) {
			NEXT_ARGUMENTS();
			NICDevice* nicdev = nicdev_get(argv[i]);
			if(!nicdev) {
				printf("Cannot Found Device!\n");
				return -2;
			}

			NEXT_ARGUMENTS();
			if(!is_uint16(argv[i])) {
				printf("VLAN ID Wrong!\n");
				return -3;
			}
			uint16_t vid = parse_uint16(argv[i]);

			if(vid == 0 || vid > 4096) {
				printf("VLAN ID Wrong!\n");
				return -3;
			}
			NICDevice* vlan_nicdev = nicdev_add_vlan(nicdev, vid);
			if(!vlan_nicdev)
				return -3;

			printf("Create VLAN NIC: %s\n", vlan_nicdev->name);
		} else if(!strcmp(argv[i], "remove")) {
			NEXT_ARGUMENTS();
			NICDevice* nicdev = nicdev_get(argv[i]);
			if(!nicdev) {
				printf("Cannot Found Device!\n");
				return -2;
			}
			if(!nicdev_remove_vlan(nicdev)) {
				printf("Cannot Remove VLAN NIC!\n");
			}
		} else if(!strcmp(argv[i], "set")) {
		} else
			return -1;
	}

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

	uint64_t perf_status = msr_read(MSR_IA32_PERF_STATUS);
	perf_status = ((perf_status & 0xff00) >> 8);
	if(!cpu_has_feature(CPU_FEATURE_TURBO_BOOST)) {
		printf("Not Support Turbo Boost\n");
		printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);

		return 0;
	}

	uint64_t perf_ctrl = msr_read(MSR_IA32_PERF_CTL);

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
			msr_write(0x10000ff00, MSR_IA32_PERF_CTL);
			printf("Turbo Boost Disabled\n");
			perf_status = msr_read(MSR_IA32_PERF_STATUS);
			perf_status = ((perf_status & 0xff00) >> 8);
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}
	} else if(!strcmp(argv[1], "on")) {
		if(perf_ctrl & 0x100000000) {
			msr_write(0xff00, MSR_IA32_PERF_CTL);
			printf("Turbo Boost Enabled\n");
			perf_status = msr_read(MSR_IA32_PERF_STATUS);
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

static int cmd_arping(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	NIC* nic = NULL;
	uint32_t addr = 0;
	uint32_t count = 1;

	if(!parse_addr(argv[1], &addr)) {
		printf("Address Wrong\n");
		return -1;
	}

	for(int i = 2; i < argc; i++) {
		if(!strcmp(argv[i], "-c")) {
			NEXT_ARGUMENTS();

			if(!is_uint32(argv[i]))
				return -i;

			count = parse_uint32(argv[i]);
		} else if(!strcmp(argv[i], "-I")) {
			NEXT_ARGUMENTS();

			uint16_t vmid;
			uint16_t vnic_index;
			uint16_t interface_index;
			if(!parse_vnic_interface(argv[i], &vmid, &vnic_index, &interface_index))
				return -i;

			if(vmid)
				return -i;

			VNIC* vnic = manager.vnics[vnic_index];
			if(!vnic)
				return -i;

			nic = vnic->nic;
		}
	}

	if(!nic) {
		return -1;
	}

	manager_arping(nic, addr, count);

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
	printf("%s\n", cmd_result);

	if(ret)
		callback(cmd_result, 0);

	return 0;
}

char* strtok(char* argv, const char* delim) {
	char* ptr = strstr(argv, delim);
	if(!ptr)
		return NULL;
	memset(ptr, 0, strlen(delim));

	return argv;
}

char* strtok_r(char* argv, const char* delim, char** next) {
	char* res = strtok(argv, delim);
	if(!res) {
		*next = NULL;
		return argv;
	}

	*next = res + strlen(res) + strlen(delim);
	return res;
}

static int cmd_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	// Default value
	VMSpec vm;
	memset(&vm, 0, sizeof(VMSpec));
	vm.core_size = 1;
	vm.memory_size = 0x1000000;		/* 16MB */
	vm.storage_size = 0x1000000;		/* 16MB */

	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;

	vm.argc = 0;
	vm.argv = malloc(sizeof(char*) * CMD_MAX_ARGC);
	memset(vm.argv, 0, sizeof(char*) * CMD_MAX_ARGC);

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-c") == 0) {
			NEXT_ARGUMENTS();

			if(!is_uint8(argv[i])) {
				printf("Core must be uint8\n");
				return -1;
			}

			vm.core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "-m") == 0) {
			NEXT_ARGUMENTS();

			if(!is_uint32(argv[i])) {
				printf("Memory must be uint32\n");
				return -1;
			}

			vm.memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "-s") == 0) {
			NEXT_ARGUMENTS();

			if(!is_uint32(argv[i])) {
				printf("Storage must be uint32\n");
				return -1;
			}

			vm.storage_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "-n") == 0) {
			NICSpec* nic = &(vm.nics[vm.nic_count++]);

			NEXT_ARGUMENTS();
			char* next;
			char* token = strtok_r(argv[i], ",", &next);
			while(token) {
				char* value;
				token = strtok_r(token, "=", &value);
				if(strcmp(token, "mac") == 0) {
					if(!is_uint64(value)) {
						printf("Mac must be uint64\n");
						return -1;
					}
					nic->mac = parse_uint64(token);
				} else if(strcmp(token, "dev") == 0) {
					nic->dev = malloc(strlen(token + 1));
					strcpy(nic->dev, value);
				} else if(strcmp(token, "ibuf") == 0) {
					if(!is_uint32(value)) {
						printf("Ibuf must be uint32\n");
						return -1;
					}
					nic->input_buffer_size = parse_uint32(value);
				} else if(strcmp(token, "obuf") == 0) {
					if(!is_uint32(value)) {
						printf("Obuf must be uint32\n");
						return -1;
					}
					nic->output_buffer_size = parse_uint32(value);
				} else if(strcmp(token, "iband") == 0) {
					if(!is_uint64(value)) {
						printf("Iband must be uint64\n");
						return -1;
					}
					nic->input_bandwidth = parse_uint64(value);
				} else if(strcmp(token, "oband") == 0) {
					if(!is_uint64(value)) {
						printf("Oband must be uint64\n");
						return -1;
					}
					nic->output_bandwidth = parse_uint64(value);
				} else if(strcmp(token, "hpad") == 0) {
					if(!is_uint16(value)) {
						printf("Iband must be uint16\n");
						return -1;
					}
					nic->padding_head = parse_uint16(value);
				} else if(strcmp(token, "tpad") == 0) {
					if(!is_uint16(value)) {
						printf("Oband must be uint16\n");
						return -1;
					}
					nic->padding_tail = parse_uint16(value);
				} else if(strcmp(token, "pool") == 0) {
					if(!is_uint32(value)) {
						printf("Pool must be uint32\n");
						return -1;
					}
					nic->pool_size = parse_uint32(value);
				} else {
					i--;
					break;
				}

				token = strtok_r(next, ",", &next);
			}
		} else if(strcmp(argv[i], "-a") == 0) {
			NEXT_ARGUMENTS();

			for( ; i < argc; i++) {
				vm.argv[vm.argc++] = argv[i];
			}
		}
	}

	uint32_t vmid = vm_create(&vm);
	if(vmid == 0) {
		callback(NULL, -1);
	} else {
		VM* vm = vm_get(vmid);

		printf("Manager: VM[%d] created(cores [\n", vm->id);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d, ", mp_apic_id_to_processor_id(vm->cores[i]));
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("], %dMBs memory, %dMBs storage, VNICs: %d\n",
				vm->memory.count * VM_MEMORY_SIZE_ALIGN / 0x100000,
				vm->storage.count * VM_STORAGE_SIZE_ALIGN / 0x100000, vm->nic_count);

		//Ok
		for(int i = 0; i < vm->nic_count; i++) {
			VNIC* vnic = vm->nics[i];
			printf("\t");
			for(int j = 5; j >= 0; j--) {
				printf("%02lx", (vnic->mac >> (j * 8)) & 0xff);
				if(j - 1 >= 0)
					printf(":");
				else
					printf(" ");
			}
			printf("%ldMbps/%ld, %ldMbps/%ld, %ldMBs\n", vnic->rx_bandwidth / 1000000, vnic->rx.size, vnic->tx_bandwidth / 1000000, vnic->tx.size, vnic->nic_size / (1024 * 1024));
		}

		printf("\targs(%d): ", vm->argc);
		for(int i = 0; i < vm->argc; i++) {
			char* ch = strchr(vm->argv[i], ' ');
			if(ch == NULL)
				printf("%s ", vm->argv[i]);
			else
				printf("\"%s\" ", vm->argv[i]);
		}
		printf("\n");

		sprintf(cmd_result, "%d", vmid);
		printf("%s\n", cmd_result);
		callback(cmd_result, 0);
	}

	for(int i = 0; i < vm.nic_count; i++) {
		if(vm.nics[i].dev)
			free(vm.nics[i].dev);
	}

	free(vm.argv);
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
		printf("Vm destroy success\n");
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
	callback(cmd_result, 0);
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

	// Attach root directory for full path
	char file_name[FILE_MAX_NAME_LEN];
	file_name[0] = '/';
	strcpy(&file_name[1], argv[2]);

	int fd = open(file_name, "r");
	if(fd < 0) {
		printf("Cannot open file: %s\n", file_name);
		return 1;
	}

	int offset = 0;
	int len;
	char buf[4096];
	while((len = read(fd, buf, 4096)) > 0) {
		if(vm_storage_write(vmid, buf, offset, len) != len) {
			printf("Upload fail : %s\n", file_name);
			callback("false", -1);
			return -1;
		}

		offset += len;
	}

	printf("Upload success : %s\n", vmid);
	callback("true", 0);
	return 0;
}


typedef struct {
	VM_STATUS_CALLBACK	callback;
	void*			context;
	int			status;
} CallbackInfo;

static void status_setted(bool result, void* context) {
	void (*callback)(char* result, int exit_status) = context;
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
		printf("%s\n: command not found\n", argv[0]);
		callback("invalid", -1);
		return -1;
	}

	shell_sync();

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
				callback("start", 0);
				break;
			case VM_STATUS_PAUSE:
				printf("pause");
				callback("pause", 0);
				break;
			case VM_STATUS_STOP:
				printf("stop");
				callback("stop", 0);
				break;
			default:
				printf("invalid");
				callback("invalid", -1);
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
			printf("stdio fail\n");
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

//TODO
static int cmd_dump(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	static uint8_t opt = 0;
	if(opt)
		opt = 0;
	else
		opt = 0xff;

	nidev_debug_switch_set(opt);

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
		.name = "interface",
		.desc = "List of virtual network interface",
		.func = cmd_interface
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
	{
		.name = "mount",
		.desc = "Mount file system",
		.args = "result: bool, [\"-t\" (type)] device: string dir: string",
		.func = cmd_mount
	},
	{
		.name = "dump",
		.desc = "Packet dump",
		.args = "TODO",
		.func = cmd_dump
	},
	{
		.name = NULL
	},
};
