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
#include "file.h"
#include "driver/charout.h"
#include "driver/charin.h"
#include "driver/disk.h"
#include "driver/nicdev.h"
#include "driver/fs.h"

#include "shell.h"

#define MAX_VM_COUNT	128

static bool cmd_sync;

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
		printf("Can'nt found manager\n");
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
		Device* dev = nicdev_get(argv[2]);
		if(!dev)
			return -2;

		uint64_t attrs[] = {
			VNIC_MAC, ((NICDevice*)dev->priv)->mac,
			VNIC_DEV, (uint64_t)argv[2],
			VNIC_NONE
		};

		/*
		 *if(vnic_update(manager_nic, attrs)) {
		 *        printf("Device not found\n");
		 *        return -3;
		 *}
		 */
		manager_set_interface();

		return 0;
	} else
		return -1;

	return 0;
}

static int cmd_nic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	extern NICDevice* nic_devices[];
	uint16_t nic_device_index = 0;

/*
 *        if(argc == 1 || (argc == 2 && !strcmp(argv[1], "list"))) {
 *                for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
 *                        NICDevice* nic_device = nic_devices[i];
 *                        if(!nic_device)
 *                                break;
 *
 *                        printf("%12s", nic_device->name);
 *                        printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
 *                                (nic_device->mac >> 40) & 0xff,
 *                                (nic_device->mac >> 32) & 0xff,
 *                                (nic_device->mac >> 24) & 0xff,
 *                                (nic_device->mac >> 16) & 0xff,
 *                                (nic_device->mac >> 8) & 0xff,
 *                                (nic_device->mac >> 0) & 0xff);
 *                }
 *
 *                return 0;
 *        }
 */

	return -1;
}

static int cmd_vnic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
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

			printf("Parent %s\n", vnic->parent);
/*
 *                        printf("%12sRX packets:%d dropped:%d\n", "", vnic->nic->input_packets, vnic->nic->input_drop_packets);
 *                        printf("%12sTX packets:%d dropped:%d\n", "", vnic->nic->output_packets, vnic->nic->output_drop_packets);
 *                        printf("%12srxqueuelen:%d txqueuelen:%d\n", "", fifo_capacity(vnic->nic->input_buffer), fifo_capacity(vnic->nic->output_buffer));
 *
 *                        printf("%12sRX bytes:%lu ", "", vnic->nic->input_bytes);
 *                        print_byte_size(vnic->nic->input_bytes);
 *                        printf("  TX bytes:%lu ", vnic->nic->output_bytes);
 *                        print_byte_size(vnic->nic->output_bytes);
 *                        printf("\n");
 *                        printf("%12sHead Padding:%d Tail Padding:%d", "",vnic->padding_head, vnic->padding_tail);
 *                        printf("\n\n");
 */
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

		return 0;
	}

	return -1;
}

static int cmd_vlan(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
/*
 *        extern Device* nic_devices[];
 *        if(argc < 3) {
 *                printf("Wrong number of arguments\n");
 *                return -1;
 *        }
 *
 *        if(!strcmp(argv[1], "add")) {
 *                if(argc != 4) {
 *                        printf("Wrong number of arguments\n");
 *                        return -1;
 *                }
 *
 *                uint16_t port = 0;
 *                Device* dev = nic_parse_index(argv[2], &port);
 *                if(!dev) {
 *                        printf("Cannot found Device\n");
 *                        return -2;
 *                }
 *
 *                if(!is_uint16(argv[3])) {
 *                        printf("VLAN ID wrong\n");
 *                        return -3;
 *                }
 *                uint16_t vid = parse_uint16(argv[3]);
 *
 *                if(vid == 0 || vid > 4096) {
 *                        printf("VLAN ID wrong\n");
 *                        return -3;
 *                }
 *
 *                Map* nics = ((NICDevice*)dev->priv)->nics;
 *                port = (port & 0xf000) | vid;
 *                if(map_contains(nics, (void*)(uint64_t)port)) {
 *                        printf("Already exist\n");
 *                        return -3;
 *                }
 *
 *                Map* vnics = map_create(8, NULL, NULL, NULL);
 *                if(!vnics) {
 *                        printf("Cannot allocate vnic map\n");
 *                        return -4;
 *                }
 *                if(!map_put(nics, (void*)(uint64_t)port, vnics)) {
 *                        printf("Cannot add VLAN");
 *                        map_destroy(vnics);
 *                        return -4;
 *                }
 *
 *                return 0;
 *        } else if(!strcmp(argv[1], "remove")) {
 *                if(argc != 3) {
 *                        printf("Wrong number of arguments\n");
 *                        return -1;
 *                }
 *
 *                uint16_t port = 0;
 *                Device* dev = nic_parse_index(argv[2], &port);
 *                if(!dev)
 *                        return -2;
 *
 *                if(!(port & 0xfff)) { //vid == 0
 *                        printf("VLan ID is 0\n");
 *                        return -2;
 *                }
 *
 *                if(!map_remove(((NICDevice*)dev->priv)->nics, (void*)(uint64_t)port)) {
 *                        printf("Cannot remove VLAN\n");
 *                        return -2;
 *                }
 *        } else
 *                return -1;
 *
 */
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

	/*
	 *if(arp_request(manager_nic->nic, arping_addr, 0)) {
	 *        arping_event = event_timer_add(arping_timeout, NULL, 1000000, 1000000);
	 *} else {
	 *        arping_count = 0;
	 *        printf("Cannot send ARP packet\n");
	 *}
	 */

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

	if(ret)
		callback(cmd_result, 0);
	return 0;
}

static int cmd_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	// Default value
	VMSpec* vm = malloc(sizeof(VMSpec));
	vm->core_size = 1;
	vm->memory_size = 0x1000000;		/* 16MB */
	vm->storage_size = 0x1000000;		/* 16MB */
	vm->nic_count = 1;
	vm->nics = malloc(sizeof(NICSpec) * MAX_VNIC_COUNT);
	vm->argc = 0;
	vm->argv = malloc(sizeof(char*) * CMD_MAX_ARGC);

	NICSpec* nic = &vm->nics[0];
	nic->mac = 0;
	nic->dev = malloc(strlen("eth0") + 1);
	nic->dev = strcpy(nic->dev, "eth0");
	nic->input_buffer_size = 1024;
	nic->output_buffer_size = 1024;
	nic->input_bandwidth = 1000000000;	/* 1 GB */
	nic->output_bandwidth = 1000000000;	/* 1 GB */
	nic->pool_size = 0x400000;		/* 4 MB */

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

			// Default value
			nic->mac = 0;
			nic->dev = malloc(strlen("eth0") + 1);
			nic->dev = strcpy(nic->dev, "eth0");
			nic->input_buffer_size = 1024;
			nic->output_buffer_size = 1024;
			nic->input_bandwidth = 1000000000; /* 1 GB */
			nic->output_bandwidth = 1000000000; /* 1 GB */
			nic->pool_size = 0x400000; /* 4 MB */

			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("Mac must be uint64\n");
						return -1;
					}
					nic->mac = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "dev:") == 0) {
					i++;
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
		printf("%s\n: command not found\n", argv[0]);
		callback("invalid", -1);
		return -1;
	}

	cmd_sync = true;
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
	{
		.name = "mount",
		.desc = "Mount file system",
		.args = "result: bool, [\"-t\" (type)] device: string dir: string",
		.func = cmd_mount
	},
	{
		.name = NULL
	},
};

static void cmd_callback(char* result, int exit_status) {
	cmd_update_var(result, exit_status);
	cmd_sync = false;
}

void shell_callback() {
	static char cmd[CMD_SIZE];
	static int cmd_idx = 0;
	extern Device* device_stdout;

	if(cmd_sync)
		return;

	int ch = stdio_getchar();
	while(ch >= 0) {
		switch(ch) {
			case '\n':
				cmd[cmd_idx] = '\0';
				putchar(ch);

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
		if(cmd_sync)
			break;

		ch = stdio_getchar();
	}
}

void shell_init() {
	printf("\nPacketNgin ver %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG);
	printf("# ");

	extern Device* device_stdin;
	((CharIn*)device_stdin->driver)->set_callback(device_stdin->id, shell_callback);
	cmd_sync = false;
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
						/*
						 *if(arp_request(manager_nic->nic, arping_addr, 0)) {
						 *        arping_event = event_timer_add(arping_timeout, NULL, 1000000, 1000000);
						 *} else {
						 *        arping_count = 0;
						 *        printf("Cannot send ARP packet\n");
						 *}
						 */

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
