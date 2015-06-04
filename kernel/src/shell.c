#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <util/types.h>
#include <util/cmd.h>
#include <util/map.h>
#include <util/event.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>

#include "stdio.h"
#include "cpu.h"
#include "version.h"
#include "rtc.h"
#include "ni.h"
#include "manager.h"
#include "device.h"
#include "port.h"
#include "acpi.h"
#include "driver/charout.h"
#include "driver/charin.h"
#include "vm.h"
#include "rootfs.h"
#include "asm.h"

#include "shell.h"

#define MAX_VM_COUNT	128
#define MAX_NIC_COUNT	32

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
	callback(cmd_result, 0);

	return 0;
}

static int cmd_sleep(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t time = 1;
	if(argc >= 2 && is_uint32(argv[1])) {
		time = parse_uint32(argv[1]);
	}
	cpu_mwait(time);
	
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

static int cmd_ip(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t old = manager_get_ip();
	if(argc == 1) {
		printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
		return 0;
	}
	
	char* str = argv[1];
	uint32_t address = (strtol(str, &str, 0) & 0xff) << 24; str++;
	address |= (strtol(str, &str, 0) & 0xff) << 16; str++;
	address |= (strtol(str, &str, 0) & 0xff) << 8; str++;
	address |= strtol(str, NULL, 0) & 0xff;
	
	manager_set_ip(address);
	
	printf("Manager's IP address changed from %d.%d.%d.%d to %d.%d.%d.%d\n", 
		(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
		(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	
	return 0;
}

static int cmd_port(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint16_t old = manager_get_port();
	if(argc == 1) {
		printf("%d\n", old);
		return 0;
	}
	
	if(!is_uint16(argv[1])) {
		return -1;
	}

	uint16_t port = parse_uint16(argv[1]);
	
	manager_set_port(port);

	printf("Manager's Port changed from %d to %d\n", old, port);
	
	return 0;
}

static int cmd_netmask(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t old = manager_get_netmask();
	if(argc == 1) {
		printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
		return 0;
	}
	
	char* str = argv[1];
	uint32_t address = (strtol(str, &str, 0) & 0xff) << 24; str++;
	address |= (strtol(str, &str, 0) & 0xff) << 16; str++;
	address |= (strtol(str, &str, 0) & 0xff) << 8; str++;
	address |= strtol(str, NULL, 0) & 0xff;
	
	manager_set_netmask(address);
	
	printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n", 
		(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
		(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	
	return 0;
}

static int cmd_gateway(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t old = manager_get_gateway();
	if(argc == 1) {
		printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
		return 0;
	}
	
	char* str = argv[1];
	uint32_t address = (strtol(str, &str, 0) & 0xff) << 24; str++;
	address |= (strtol(str, &str, 0) & 0xff) << 16; str++;
	address |= (strtol(str, &str, 0) & 0xff) << 8; str++;
	address |= strtol(str, NULL, 0) & 0xff;
	
	manager_set_gateway(address);
	
	printf("Manager's Gateway changed from %d.%d.%d.%d to %d.%d.%d.%d\n", 
		(old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff,
		(address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	
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
	if(!CPU_IS_TURBO_BOOST) {
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

static int cmd_lsni(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	extern Map* nis;
	extern uint64_t ni_mac;
	
	MapIterator iter;
	map_iterator_init(&iter, nis);
	while(map_iterator_has_next(&iter)) {
		NI* ni = map_iterator_next(&iter)->data;
		
		printf("%02x:%02x:%02x:%02x:%02x:%02x %c\n", 
			(ni->mac >> 40) & 0xff, 
			(ni->mac >> 32) & 0xff, 
			(ni->mac >> 24) & 0xff, 
			(ni->mac >> 16) & 0xff, 
			(ni->mac >> 8) & 0xff, 
			(ni->mac >> 0) & 0xff, 
			ni->mac == ni_mac ? '*' : ' ');
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
	
	arping_time = cpu_tsc();
	
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
	
	char* str = argv[1];
	arping_addr = (strtol(str, &str, 0) & 0xff) << 24; str++;
	arping_addr |= (strtol(str, &str, 0) & 0xff) << 16; str++;
	arping_addr |= (strtol(str, &str, 0) & 0xff) << 8; str++;
	arping_addr |= strtol(str, NULL, 0) & 0xff;
	
	arping_time = cpu_tsc();
	
	arping_count = 1;
	if(argc >= 4) {
		if(strcmp(argv[2], "-c") == 0) {
			arping_count = strtol(argv[3], NULL, 0);
		}
	}
	
	if(arp_request(manager_ni->ni, arping_addr)) {
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
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	VMSpec* vm = malloc(sizeof(VMSpec));
	vm->core_size = 1;
	vm->memory_size = 0x1000000;	// 16MB
	vm->storage_size = 0x1000000;	// 16MB
	vm->nic_count = 0;
	vm->nics = malloc(sizeof(NICSpec) * MAX_NIC_COUNT);
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
			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("mac must be uint64\n");
						return -1;
					}
					nic->mac = parse_uint64(argv[i]);
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
	
	uint32_t vmid = vm_create(vm);
	if(vmid == 0) {
		callback(NULL, -1);
	} else {
		sprintf(cmd_result, "%d", vmid);
		callback(cmd_result, 0);
	}
	return 0;
}

static int cmd_vm_delete(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 1) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	bool ret = vm_delete(vmid);

	if(ret)
		callback("true", 0);
	else
		callback("false", -1);

	return 0;
}

static int cmd_vm_list(int argc, char** argv, void(*callback)(char* result, int eixt_status)) {
	uint32_t vmids[MAX_VM_COUNT];
	int len = vm_list(vmids, MAX_VM_COUNT);

	char* p = cmd_result;
	for(int i = 0; i < len; i++) {
		p += sprintf(p, "%lu", vmids[i]) - 1;
		if(i + 1 < len) {
			*p++ = ' ';
		} else {
			*p++ = '\0';
		}
	}

	callback(cmd_result, 0);
	return 0;
}

static int cmd_send(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	uint32_t size = 0;
	void* file = rootfs_file(argv[2], &size);
	
	if(file == NULL) {
		printf("Cannot open file: %s\n", argv[2]);
		return 1;
	}

	if(vm_storage_write(vmid, file, 0, size) != size) {
		callback("false", -1);
		return -1;
	}
	
	callback("true", 0);
	return 0;
}

static int cmd_status_set(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	void status_setted(bool result, void* context) {
		void(*callback)(char* result, int exit_status) = context;
		callback(result ? "true" : "false", 0);
	}

	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	if(!is_uint32(argv[1])) {
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
	}
	
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
	switch(vm_status_get(vmid)) {
		case VM_STATUS_START:
			callback("start", 0);
			break;
		case VM_STATUS_PAUSE:
			callback("pause", 0);
			break;
		case VM_STATUS_STOP:
			callback("stop", 0);
			break;
		default:
			callback("invalid", -1);
			break;
	}

	return 0;
}

Command commands[] = {
	{
		.name = "help",
		.desc = "Show this message.", 
		.func = cmd_help 
	},
	{ 
		.name = "version", 
		.desc = "Print the kernel version.", 
		.func = cmd_version 
	},
	{
		.name = "turbo",
		.desc = "Turbo Boost Enable/Disable",
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
		.args = "[time: uint32]",
		.func = cmd_sleep
	},
	{ 
		.name = "date", 
		.desc = "Print current date and time.", 
		.func = cmd_date 
	},
	{ 
		.name = "ip", 
		.desc = "Get or set IP address of manager.",
		.args = "[(address)]",
		.func = cmd_ip 
	},
	{ 
		.name = "port", 
		.desc = "Get or set Port of manager.",
		.args = "[(port)]",
		.func = cmd_port 
	},
	{ 
		.name = "netmask", 
		.desc = "Get or set Netmask of manager.",
		.args = "[(netmask)]",
		.func = cmd_netmask
	},
	{ 
		.name = "gateway", 
		.desc = "Get or set Gateway of manager.",
		.args = "[(address)]",
		.func = cmd_gateway
	},
	{ 
		.name = "lsni", 
		.desc = "List network interfaces.", 
		.func = cmd_lsni 
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
		.name = "delete",
		.desc = "Delete VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_vm_delete
	},
	{
		.name = "list",
		.desc = "List VM",
		.args = "result: uint64[]",
		.func = cmd_vm_list
	},
	{
		.name = "send",
		.desc = "Send file",
		.args = "result: bool, vmid: uint32 path: string",
		.func = cmd_send
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
		.name = NULL
	},
};

static void cmd_callback(char* result, int exit_status) {
	cmd_update_var(result, exit_status);
	printf("%s\n", result);
}

void shell_callback(int code) {
	static char cmd[CMD_SIZE];
	static int cmd_idx = 0;
	extern Device* device_stdout;
	stdio_scancode(code);
	
	int ch = stdio_getchar();
	while(ch >= 0) {
		switch(ch) {
			case '\n':
				cmd[cmd_idx] = '\0';
				putchar(ch);
				cmd_exec(cmd, cmd_callback);
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
		
		ch = stdio_getchar();
	}
}

void shell_init() {
	printf("\nPacketNgin ver %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG);
	printf("# ");
	
	extern Device* device_stdin;
	((CharIn*)device_stdin->driver)->set_callback(device_stdin->id, shell_callback);
	cmd_init();
}

bool shell_process(Packet* packet) {
	if(arping_count== 0)
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
				uint64_t time = cpu_tsc();
				uint32_t ms = (time - arping_time) / cpu_ms;
				uint32_t ns = (time - arping_time) / cpu_ns - ms * 1000;
				
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
						arping_time = cpu_tsc();
						if(arp_request(manager_ni->ni, arping_addr)) {
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
