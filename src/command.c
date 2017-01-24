#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <util/cmd.h>
#include <util/list.h>
#include <util/types.h>
#include <util/map.h>

#include "vm.h"
#include "vnic.h"
#include "file.h"
#include "dispatcher.h"
#include "version.h"
#include "manager.h"
#include "gmalloc.h"
#include "driver/nic.h"

bool cmd_sync;

/*
 *static void usage(const char* cmd) {
 *        printf("\nUsage :\n");
 *        for(int i = 0; commands[i].name != NULL; i++) {
 *                if(strcmp(cmd, commands[i].name) == 0) {
 *                        printf("\t%s %s\n\n", cmd, commands[i].args);
 *                }
 *        }
 *}
 *
 *static int cmd_clear(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
 *        printf(">");
 *
 *        return 0;
 *}
 *
 *static int cmd_echo(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
 *        int pos = 0;
 *        for(int i = 1; i < argc; i++) {
 *                pos += sprintf(cmd_result + pos, "%s", argv[i]) - 1;
 *                if(i + 1 < argc) {
 *                        cmd_result[pos++] = ' ';
 *                }
 *        }
 *        callback(cmd_result, 0);
 *
 *        return 0;
 *}
 *
 *static int cmd_sleep(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
 *        uint32_t time = 1;
 *        if(argc >= 2 && is_uint32(argv[1])) {
 *                time = parse_uint32(argv[1]);
 *        }
 *        sleep(time);
 *        
 *        return 0;
 *}
 */

/*static char* months[] = {*/
	/*"???",*/
	/*"Jan",*/
	/*"Feb",*/
	/*"Mar",*/
	/*"Apr",*/
	/*"May",*/
	/*"Jun",*/
	/*"Jul",*/
	/*"Aug",*/
	/*"Sep",*/
	/*"Oct",*/
	/*"Nov",*/
	/*"Dec",*/
/*};*/

/*static char* weeks[] = {*/
	/*"???",*/
	/*"Sun",*/
	/*"Mon",*/
	/*"Tue",*/
	/*"Wed",*/
	/*"Thu",*/
	/*"Fri",*/
	/*"Sat",*/
/*};*/

/*static int cmd_date(int argc, char** argv, void(*callback)(char* result, int exit_status)) {*/
	/*uint32_t date = rtc_date();*/
	/*uint32_t time = rtc_time();*/
	
	/*printf("%s %s %d %02d:%02d:%02d UTC %d\n", weeks[RTC_WEEK(date)], months[RTC_MONTH(date)], RTC_DATE(date),*/
		/*RTC_HOUR(time), RTC_MINUTE(time), RTC_SECOND(time), 2000 + RTC_YEAR(date));*/
	
	/*return 0;*/
/*}*/

/*
 *static bool parse_addr(char* argv, uint32_t* address) {
 *        char* next = NULL;
 *        uint32_t temp;
 *        temp = strtol(argv, &next, 0);
 *        if(temp > 0xff)
 *                return false;
 *
 *        *address = (temp & 0xff) << 24;
 *        if(next == argv)
 *                return false;
 *        argv = next;
 *
 *        if(*argv != '.')
 *                return false;
 *        argv++;
 *        temp = strtol(argv, &next, 0);
 *        if(temp > 0xff)
 *                return false;
 *
 *        *address |= (temp & 0xff) << 16;
 *        if(next == argv)
 *                return false;
 *        argv = next;
 *
 *        if(*argv != '.')
 *                return false;
 *        argv++;
 *        temp = strtol(argv, &next, 0);
 *        if(temp > 0xff)
 *                return false;
 *        *address |= (temp & 0xff) << 8;
 *        if(next == argv)
 *                return false;
 *        argv = next;
 *
 *        if(*argv != '.')
 *                return false;
 *        argv++;
 *        temp = strtol(argv, &next, 0);
 *        if(temp > 0xff)
 *                return false;
 *        *address |= temp & 0xff;
 *        if(next == argv)
 *                return false;
 *        argv = next;
 *
 *        if(*argv != '\0')
 *                return false;
 *
 *        return true;
 *}
 */

/*
 *static int cmd_manager(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
 *        if(manager_nic == NULL) {
 *                printf("Can'nt found manager\n");
 *                return -1;
 *        }
 *
 *        if(argc == 1) {
 *                printf("HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
 *                        (manager_nic->mac >> 40) & 0xff,
 *                        (manager_nic->mac >> 32) & 0xff,
 *                        (manager_nic->mac >> 24) & 0xff,
 *                        (manager_nic->mac >> 16) & 0xff,
 *                        (manager_nic->mac >> 8) & 0xff,
 *                        (manager_nic->mac >> 0) & 0xff);
 *                uint32_t ip = manager_get_ip();
 *                printf("%10sinet addr:%d.%d.%d.%d  ", "", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip >> 0) & 0xff);
 *                uint16_t port = manager_get_port();
 *                printf("port:%d\n", port);
 *                uint32_t mask = manager_get_netmask();
 *                printf("%10sMask:%d.%d.%d.%d\n", "", (mask >> 24) & 0xff, (mask >> 16) & 0xff, (mask >> 8) & 0xff, (mask >> 0) & 0xff);
 *                uint32_t gw = manager_get_gateway();
 *                printf("%10sGateway:%d.%d.%d.%d\n", "", (gw >> 24) & 0xff, (gw >> 16) & 0xff, (gw >> 8) & 0xff, (gw >> 0) & 0xff);
 *
 *                return 0;
 *        }
 *
 *        if(!manager_nic) {
 *                printf("Can'nt found manager\n");
 *                return -1;
 *        }
 *
 *        if(!strcmp("ip", argv[1])) {
 *                uint32_t old = manager_get_ip();
 *                if(argc == 2) {
 *                        printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
 *                        return 0;
 *                }
 *
 *                uint32_t address;
 *                if(!parse_addr(argv[2], &address)) {
 *                        printf("address wrong\n");
 *                        return 0;
 *                }
 *
 *                manager_set_ip(address);
 *        } else if(!strcmp("port", argv[1])) {
 *                uint16_t old = manager_get_port();
 *                if(argc == 2) {
 *                        printf("%d\n", old);
 *                        return 0;
 *                }
 *
 *                if(!is_uint16(argv[2])) {
 *                        printf("port number wrong\n");
 *                        return -1;
 *                }
 *
 *                uint16_t port = parse_uint16(argv[2]);
 *
 *                manager_set_port(port);
 *        } else if(!strcmp("netmask", argv[1])) {
 *                uint32_t old = manager_get_netmask();
 *                if(argc == 2) {
 *                        printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
 *                        return 0;
 *                }
 *
 *                uint32_t address;
 *                if(!parse_addr(argv[2], &address)) {
 *                        printf("address wrong\n");
 *                        return 0;
 *                }
 *
 *                manager_set_netmask(address);
 *
 *                printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n",
 *                        (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
 *                        (address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
 *
 *        } else if(!strcmp("gateway", argv[1])) {
 *                uint32_t old = manager_get_gateway();
 *                if(argc == 2) {
 *                        printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
 *                        return 0;
 *                }
 *
 *                uint32_t address;
 *                if(!parse_addr(argv[2], &address)) {
 *                        printf("address wrong\n");
 *                        return 0;
 *                }
 *
 *                manager_set_gateway(address);
 *
 *                printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n",
 *                        (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
 *                        (address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
 *        } else if(!strcmp("nic", argv[1])) {
 *                if(argc == 2) {
 *                        printf("Network Interface name required\n");
 *                        return false;
 *                }
 *
 *                if(argc != 3) {
 *                        printf("Wrong Parameter\n");
 *                        return false;
 *                }
 *
 *                uint16_t port = 0;
 *                Device* dev = nic_parse_index(argv[2], &port);
 *                if(!dev)
 *                        return -2;
 *
 *                uint64_t attrs[] = {
 *                        NIC_MAC, ((NICPriv*)dev->priv)->mac[port >> 12],
 *                        NIC_DEV, (uint64_t)argv[2],
 *                        NIC_NONE
 *                };
 *
 *                if(vnic_update(manager_nic, attrs)) {
 *                        printf("Can'nt found device\n");
 *                        return -3;
 *                }
 *                manager_set_interface();
 *
 *                return 0;
 *        } else
 *                return -1;
 *
 *        return 0;
 *}
 *
 */
static int cmd_nic(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	extern Device* nic_devices[];
	uint16_t nic_device_index = 0;

	if(argc == 1) {
		for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
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

				char name_buf[32]= {};
				if(!vlan_id) {
					sprintf(name_buf, "eth%d", nic_device_index + port_num);
				} else {
					sprintf(name_buf, "eth%d.%d", nic_device_index + port_num, vlan_id);
				}

				printf("%-12s", name_buf);
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
		for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
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

				char name_buf[32]= {};
				if(!vlan_id) {
					sprintf(name_buf, "eth%d", nic_device_index + port_num);
				} else {
					sprintf(name_buf, "eth%d.%d", nic_device_index + port_num, vlan_id);
				}

				if(!strncmp(name_buf, argv[1], sizeof(name_buf))) {
					printf("%-12s", name_buf);
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

			printf("%-12s", name_buf);
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
			// FIXME: can't use float. It seems to be related to our own startup file
			//print_byte_size(vnic->nic->input_bytes);
			printf("  TX bytes:%lu ", vnic->nic->output_bytes);
			//print_byte_size(vnic->nic->output_bytes);
			printf("\n");
			printf("%12sHead Padding:%d Tail Padding:%d", "",vnic->padding_head, vnic->padding_tail);
			printf("\n\n");
		}

// 		print_vnic(manager_nic, 0, 0);

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
			printf("Can'nt found Device\n");
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

		Map* vnics = map_create(8, NULL, NULL, gmalloc_pool);
		if(!vnics) {
			printf("Can'nt allocate vnic map\n");
			return -4;
		}
		if(!map_put(nics, (void*)(uint64_t)port, vnics)) {
			printf("Can'nt add VLAN");
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
			printf("VLAN ID is 0\n");
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

/*
 *static int cmd_turbo(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
 *        if(argc > 2) {
 *                return CMD_STATUS_WRONG_NUMBER;
 *        }
 *
 *        uint64_t perf_status = read_msr(0x00000198);
 *        perf_status = ((perf_status & 0xff00) >> 8);
 *        if(!cpu_has_feature(CPU_FEATURE_TURBO_BOOST)) {
 *                printf("Not Support Turbo Boost\n");
 *                printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
 *
 *                return 0;
 *        }
 *
 *        uint64_t perf_ctrl = read_msr(0x00000199);
 *
 *        if(argc == 1) {
 *                if(perf_ctrl & 0x100000000) {
 *                        printf("Turbo Boost Disabled\n");
 *                        printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
 *                } else {
 *                        printf("Turbo Boost Enabled\n");
 *                        printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
 *                }
 *
 *                return 0;
 *        }
 *
 *        if(!strcmp(argv[1], "off")) {
 *                if(perf_ctrl & 0x100000000) {
 *                        printf("Turbo Boost Already Disabled\n");
 *                        printf("\tPerfomance status : %d.%d Ghz\n",  perf_status / 10, perf_status % 10);
 *                } else {
 *                        write_msr(0x00000199, 0x10000ff00);
 *                        printf("Turbo Boost Disabled\n");
 *                        perf_status = read_msr(0x00000198);
 *                        perf_status = ((perf_status & 0xff00) >> 8);
 *                        printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
 *                }
 *        } else if(!strcmp(argv[1], "on")) {
 *                if(perf_ctrl & 0x100000000) {
 *                        write_msr(0x00000199, 0xff00);
 *                        printf("Turbo Boost Enabled\n");
 *                        perf_status = read_msr(0x00000198);
 *                        perf_status = ((perf_status & 0xff00) >> 8);
 *                        printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
 *                } else {
 *                        printf("Turbo Boost Already Enabled\n");
 *                        printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
 *                }
 *        } else {
 *                return -1;
 *        }
 *
 *        return 0;
 *}
 *
 *static int cmd_reboot(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
 *        asm volatile("cli");
 *        
 *        uint8_t code;
 *        do {
 *                code = port_in8(0x64);	// Keyboard Control
 *                if(code & 0x01)
 *                        port_in8(0x60);	// Keyboard I/O
 *        } while(code & 0x02);
 *        
 *        port_out8(0x64, 0xfe);	// Reset command
 *        
 *        while(1)
 *                asm("hlt");
 *        
 *        return 0;
 *}
 */

static int cmd_shutdown(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	printf("Shutting down\n");
	dispatcher_exit();
	exit(EXIT_SUCCESS);
//	acpi_shutdown();
	
	return 0;
}

/*static uint32_t arping_addr;*/
/*static uint32_t arping_count;*/
/*static uint64_t arping_time;*/
/*static uint64_t arping_event;*/

/*static bool arping_timeout(void* context) {*/
	/*if(arping_count <= 0)*/
		/*return false;*/
	
	/*arping_time = timer_ns();*/
	
	/*printf("Reply timeout\n");*/
	/*arping_count--;*/
	
	/*if(arping_count > 0) {*/
		/*return true;*/
	/*} else {*/
		/*printf("Done\n");*/
		/*return false;*/
	/*}*/
/*}*/

/*static int cmd_arping(int argc, char** argv, void(*callback)(char* result, int exit_status)) {*/
	/*if(argc < 2) {*/
		/*return CMD_STATUS_WRONG_NUMBER;*/
	/*}*/
	
	/*if(!parse_addr(argv[1], &arping_addr)) {*/
		/*printf("Address Wrong\n");*/
		/*return -1;*/
	/*}*/
	
	/*arping_time = timer_ns();*/
	
	/*arping_count = 1;*/
	/*if(argc >= 4) {*/
		/*if(strcmp(argv[2], "-c") == 0) {*/
			/*arping_count = strtol(argv[3], NULL, 0);*/
		/*}*/
	/*}*/
	
	/*if(arp_request(manager_nic->nic, arping_addr, 0)) {*/
		/*arping_event = event_timer_add(arping_timeout, NULL, 1000000, 1000000);*/
	/*} else {*/
		/*arping_count = 0;*/
		/*printf("Cannot send ARP packet\n");*/
	/*}*/

	/*return 0;*/
/*}*/

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
		printf("Can'nt md5 checksum\n");
	} else {
		char* p = (char*)cmd_result;
		for(int i = 0; i < 16; i++, p += 2) {
			sprintf(p, "%02x", ((uint8_t*)md5sum)[i]);
		}
		*p = '\0';
	}

	if(ret) {
		printf("%s\n", cmd_result);
		callback(cmd_result, 0);
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
				printf("core must be uint8\n");
				return -1;
			}
			
			vm->core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "memory:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("memory must be uint32\n");
				return -1;
			}
			
			vm->memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "storage:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("storage must be uint32\n");
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
						printf("mac must be uint64\n");
						return -1;
					}
					nic->mac = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "dev:") == 0) {
					i++;
					if(nic->dev)
						free(nic->dev);

					nic->dev = malloc(strlen(argv[i] + 1));
					strcpy(nic->dev, argv[i]);
					printf("Device alloc %s \n", argv[i]);
				} else if(strcmp(argv[i], "ibuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("ibuf must be uint32\n");
						return -1;
					}
					nic->input_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "obuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("obuf must be uint32\n");
						return -1;
					}
					nic->output_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "iband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("iband must be uint64\n");
						return -1;
					}
					nic->input_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "oband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("oband must be uint64\n");
						return -1;
					}
					nic->output_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "hpad:") == 0) {
					i++;
					if(!is_uint16(argv[i])) {
						printf("iband must be uint16\n");
						return -1;
					}
					nic->padding_head = parse_uint16(argv[i]);
				} else if(strcmp(argv[i], "tpad:") == 0) {
				i++;
					if(!is_uint16(argv[i])) {
						printf("oband must be uint16\n");
						return -1;
					}
					nic->padding_tail = parse_uint16(argv[i]);
				} else if(strcmp(argv[i], "pool:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("pool must be uint32\n");
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
		callback("false", -1);
	} else {
		sprintf(cmd_result, "%d", vmid);
		printf("%d\n", vmid);
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

	if(ret)
		callback("true", 0);
	else
		callback("false", -1);

	return 0;
}

static int cmd_vm_list(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t vmids[MAX_VM_COUNT];
	int len = vm_list(vmids, MAX_VM_COUNT);

	char* p = cmd_result;
	for(int i = 0; i < len; i++) {
		p += sprintf(p, "%u", vmids[i]); // NOTE: sprintf return 1 not 2 //- 1;
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

	int fd = open(argv[2], O_RDONLY);
	if(fd < 0) {
		printf("Cannot open file: %s\n", argv[2]);
		return -1;
	}

	int offset = 0;
	int len;
	char buf[4096];
	while((len = read(fd, buf, 4096)) > 0) {
		if(vm_storage_write(vmid, buf, offset, len) != len) {
			close(fd);
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
	VM* vm = vm_get(vmid); 
	if(!vm) {
		printf("VM not found\n");
		return -1;
	}

	void print_vm_status(int status) {
		switch(status) {
			case VM_STATUS_START:
				callback("start", 0);
				printf("start\n");
				break;
			case VM_STATUS_PAUSE:
				callback("pause", 0);
				printf("pause\n");
				break;
			case VM_STATUS_STOP:
				callback("stop", 0);
				printf("stop\n");
				break;
			default:
				callback("invalid", -1);
				printf("invalid\n");
				break;
		}
	}

	printf("VM ID: %d\n", vmid);
	printf("Status: ");
	print_vm_status(vm->status);
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

/*static int cmd_mount(int argc, char** argv, void(*callback)(char* result, int exit_status)) {*/
	/*if(argc < 5) {*/
		/*printf("Argument is not enough\n");*/
		/*return -1;*/
	/*}*/

	/*uint8_t type;*/
	/*uint8_t disk;*/
	/*uint8_t number;*/
	/*uint8_t partition;*/

	/*if(strcmp(argv[1], "-t") != 0) {*/
		/*printf("Argument is wrong\n");*/
		/*return -2;*/
	/*}*/

	/*if(strcmp(argv[2], "bfs") == 0 || strcmp(argv[2], "BFS") == 0) {*/
		/*type = FS_TYPE_BFS;*/
	/*} else if(strcmp(argv[2], "ext2") == 0 || strcmp(argv[2], "EXT2") == 0) {*/
		/*type = FS_TYPE_EXT2;*/
	/*} else if(strcmp(argv[2], "fat") == 0 || strcmp(argv[2], "FAT") == 0) {*/
		/*type = FS_TYPE_FAT;*/
	/*} else {*/
		/*printf("%s type is not supported\n", argv[2]);*/
		/*return -3;*/
	/*}*/

	/*if(argv[3][0] == 'v') {*/
		/*disk = DISK_TYPE_VIRTIO_BLK;*/
	/*} else if(argv[3][0] == 's') {*/
		/*disk = DISK_TYPE_USB;*/
	/*} else if(argv[3][0] == 'h') {*/
		/*disk = DISK_TYPE_PATA;*/
	/*} else {*/
		/*printf("%c type is not supported\n", argv[3][0]);*/
		/*return -3;*/
	/*}*/

	/*number = argv[3][2] - 'a';*/
	/*if(number > 8) {*/
		/*printf("Disk number cannot exceed %d\n", DISK_AVAIL_DEVICES);*/
		/*return -4;*/
	/*}*/

	/*partition = argv[3][3] - '1';*/
	/*if(partition > 3) {*/
		/*printf("Partition number cannot exceed 4\n");*/
		/*return -5;*/
	/*}*/

	/*fs_mount(disk << 16 | number, partition, type, argv[4]);*/

	/*return -2;*/
/*}*/

Command commands[] = {
	{
		.name = "help",
		.desc = "Show this message.",
		.func = cmd_help
	},
/*#ifdef TEST*/
	/*{*/
		/*.name = "test",*/
		/*.desc = "Run run-time tests.", */
		/*.func = cmd_test*/
	/*},*/
/*#endif*/
	{
		.name = "version",
		.desc = "Print the kernel version.",
		.func = cmd_version
	},
	/*{*/
		/*.name = "turbo",*/
		/*.desc = "Turbo Boost Enable/Disable",*/
		/*.args = "[on/off]",*/
		/*.func = cmd_turbo*/
	/*},*/
	/*
	 *{
	 *        .name = "clear",
	 *        .desc = "Clear screen.",
	 *        .func = cmd_clear
	 *},
	 *{
	 *        .name = "echo",
	 *        .desc = "Echo arguments.",
	 *        .args = "[variable: string]*",
	 *        .func = cmd_echo
	 *},
	 *{
	 *        .name = "sleep",
	 *        .desc = "Sleep n seconds",
	 *        .args = "[n: uint32]",
	 *        .func = cmd_sleep
	 *},
	 */
	/*{ */
		/*.name = "date", */
		/*.desc = "Print current date and time.", */
		/*.func = cmd_date */
	/*},*/
	/*{ */
		/*.name = "manager",*/
		/*.desc = "Set ip, port, netmask, gateway, nic of manager",*/
		/*.func = cmd_manager*/
	/*},*/
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
	/*{ */
		/*.name = "reboot",*/
		/*.desc = "Reboot the node.",*/
		/*.func = cmd_reboot*/
	/*},*/
	{
		.name = "shutdown",
		.desc = "Shutdown the node.",
		.func = cmd_shutdown
	},
	/*{ */
		/*.name = "arping", */
		/*.desc = "ARP ping to the host.",*/
		/*.args = "(address) [\"-c\" (count)]",*/
		/*.func = cmd_arping */
	/*},*/
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
	/*{*/
		/*.name = "mount",*/
		/*.desc = "Mount file system",*/
		/*.args = "result: bool, [\"-t\" (type)] device: string dir: string",*/
		/*.func = cmd_mount*/
	/*},*/
	{
		.name = NULL
	},
};

static void cmd_callback(char* result, int exit_status) {
	if(!result)
		return;
	cmd_update_var(result, exit_status);
	cmd_sync = false;
}

static int execute_cmd(char* line, bool is_dump) {
	// if is_dump == true then file cmd
	//    is_dump == false then stdin cmd
	if(is_dump == true)
		printf("%s\n", line);
	
	int exit_status = cmd_exec(line, cmd_callback);

	if(exit_status != 0) {
		if(exit_status == CMD_STATUS_WRONG_NUMBER) {
			printf("Wrong number of arguments\n"); 
		} else if(exit_status == CMD_STATUS_NOT_FOUND) {
			printf("Command not found: %s\n", line);
		} else if(exit_status < 0) {
			printf("Error code : %d\n", exit_status);
		} else if(exit_status == CMD_VARIABLE_NOT_FOUND) {
			printf("Variable not found\n");
		} else if(exit_status < 0) { 
			printf("Wrong value of argument: %d\n", -exit_status);
		}
	}
	printf("> ");
	fflush(stdout);

	return exit_status;
}

#define MAX_LINE_SIZE		2048

void command_process(int fd) {
	char line[MAX_LINE_SIZE] = {0, };
	char* head;
	int seek = 0;
	int eod = 0; // End of data

	while((eod += read(fd, &line[eod], MAX_LINE_SIZE - eod))) {
		head = line;
		for(; seek < eod; seek++) {
			if(line[seek] == '\n') {
				line[seek] = '\0';
				int ret = execute_cmd(head, fd != STDIN_FILENO);
				
				if(ret == 0) {
					head = &line[seek] + 1;
				} else { 
					eod = 0;
					return;
				}
			}
		}
		if(head == line && eod == MAX_LINE_SIZE){ // Unfound '\n' and head == 0
			printf("Command line is too long %d > %d\n", eod, MAX_LINE_SIZE);
			eod = 0;
			return;
		 } else { // Unfound '\n' and seek != 0
			memmove(line, head, eod - (head - line));
			eod -= head - line;
			seek = eod;
			if(fd == STDIN_FILENO) {
				return;
			} else
				continue;
		}
	}

	if(eod != 0) {
		line[eod] = '\0';
		execute_cmd(&line[0], fd != STDIN_FILENO);
	}
	eod = 0;
}

