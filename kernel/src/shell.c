#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

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

#include "shell.h"

#define CMD_SIZE 	512

static int cmd_clear() {
	printf("\f");

	return 0;
}

static int cmd_echo(int argc, char** argv) {
	int pos = 0;
	for(int i = 1; i < argc; i++) {
		pos += sprintf(cmd_result + pos, "%s", argv[i]) - 1;
		if(i + 1 < argc) {
			cmd_result[pos++] = ' ';
		}
	}

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

static int cmd_date(int argc, char** argv) {
	uint32_t date = rtc_date();
	uint32_t time = rtc_time();
	
	printf("%s %s %d %02d:%02d:%02d UTC %d\n", weeks[RTC_WEEK(date)], months[RTC_MONTH(date)], RTC_DATE(date), 
		RTC_HOUR(time), RTC_MINUTE(time), RTC_SECOND(time), 2000 + RTC_YEAR(date));
	
	return 0;
}

static int cmd_ip(int argc, char** argv) {
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

static int cmd_version(int argc, char** argv) {
	printf("%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
	
	return 0;
}

static int cmd_lsni() {
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

static int cmd_reboot() {
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

static int cmd_shutdown() {
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

static int cmd_arping(int argc, char** argv) {
	if(argc < 2) {
		return 1;
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

static int cmd_create(int argc, char** argv) {
	if(argc < 2) {
		return 1;
	}

	return 0;
}

static int cmd_start(int argc, char** argv) {
	if(argc < 2) {
		return 1;
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
		.name = "clear", 
		.desc = "Clear screen.", 
		.func = cmd_clear 
	},
	{ 
		.name = "echo", 
		.desc = "Echo arguments.", 
		.func = cmd_echo 
	},
	{ 
		.name = "date", 
		.desc = "Print current date and time.", 
		.func = cmd_date 
	},
	{ 
		.name = "ip", 
		.desc = "[(address)] Get or set IP address of manager.", 
		.func = cmd_ip 
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
		.desc = "(address) [\"-c\" (count)] ARP ping to the host.", 
		.func = cmd_arping 
	},
	{ 
		.name = "create", 
		.desc = "Create VM", 
		.func = cmd_create 
	},
	{ 
		.name = "start", 
		.desc = "Start VM", 
		.func = cmd_start 
	},
	{
		.name = NULL
	},
};


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
				cmd_exec(cmd);
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
	printf("\nPacketNgin ver %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
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
