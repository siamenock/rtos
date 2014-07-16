#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <util/map.h>

#include "stdio.h"
#include "version.h"
#include "rtc.h"
#include "ni.h"
#include "manager.h"
#include "device.h"
#include "port.h"
#include "driver/charout.h"
#include "driver/charin.h"

#include "shell.h"

#define CMD_SIZE	512
#define ARG_SIZE	64

static char cmd[CMD_SIZE];
static int args[ARG_SIZE];
static int arg_idx;
static int cmd_idx;

typedef struct {
	char* label;
	char* description;
	int (*func)();
} Command;

static int command_help();

static int command_clear() {
	printf("\f");
	return 0;
}

static int command_echo() {
	int i;
	for(i = 1; i < arg_idx; i++) {
		printf("%s", cmd + args[i]);
		if(i + 1 < arg_idx)
			printf(" ");
	}
	printf("\n");
	
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

static int command_date() {
	uint32_t date = rtc_date();
	uint32_t time = rtc_time();
	
	printf("%s %s %d %02d:%02d:%02d UTC %d\n", weeks[RTC_WEEK(date)], months[RTC_MONTH(date)], RTC_DATE(date), 
		RTC_HOUR(time), RTC_MINUTE(time), RTC_SECOND(time), 2000 + RTC_YEAR(date));
	
	return 0;
}

static int command_ip() {
	uint32_t old = manager_get_ip();
	if(arg_idx == 1) {
		printf("%d.%d.%d.%d\n", (old >> 24) & 0xff, (old >> 16) & 0xff, (old >> 8) & 0xff, (old >> 0) & 0xff);
		return 0;
	}
	
	char* str = cmd + args[1];
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

static int command_version() {
	printf("%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
	
	return 0;
}

static int command_lsni() {
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

static int command_reboot() {
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

static int command_shutdown() {
	/*
	apic_enable();
	
	port_out32(
   // send the shutdown command
      outw((unsigned int) PM1a_CNT, SLP_TYPa | SLP_EN );
         if ( PM1b_CNT != 0 )
	       outw((unsigned int) PM1b_CNT, SLP_TYPb | SLP_EN );

	          wrstr("acpi poweroff failed.\n");
	*/
	return 0;
}

static Command commands[] = {
	{ "help", "Show this message.", command_help },
	{ "version", "Print the kernel version.", command_version },
	{ "clear", "Clear screen.", command_clear },
	{ "echo", "Echo arguments.", command_echo },
	{ "date", "Print current date and time.", command_date },
	{ "ip", "Change manager's IP address. You can use decimal, hexadecimal or octal.", command_ip },
	{ "lsni", "List network interfaces.", command_lsni },
	{ "reboot", "Reboot the node..", command_reboot },
	{ "shutdown", "Shutdown the node..", command_shutdown },
};

static int command_help() {
	int i;
	for(i = 0; i < sizeof(commands) / sizeof(Command); i++) {
		printf("%s\t%s\n", commands[i].label, commands[i].description);
	}
	return 0;
}

static void exec() {
	cmd[cmd_idx] = 0;
	
	int i;
	for(i = 0; i < ARG_SIZE; i++) {
		args[i] = -1;
	}
	
	bool is_start = true;
	char quotation = 0;
	arg_idx = 0;
	for(i = 0; i < cmd_idx; i++) {
		if(quotation) {
			if(cmd[i] == quotation) {
				quotation = 0;
				cmd[i] = 0;
				cmd[i + 1] = 0;
				i++;
				is_start = true;
			}
		} else {
			switch(cmd[i]) {
				case '"': case '\'':
					quotation = cmd[i];
					cmd[i] = 0;
					i++;
					args[arg_idx++] = i;
					break;
				case ' ':
					cmd[i] = 0;
					is_start = true;
					break;
				default:
					if(is_start) {
						args[arg_idx++] = i;
						is_start = false;
					}
			}
		}
	}
	
	for(i = 0; i < sizeof(commands) / sizeof(Command); i++) {
		if(strcmp(cmd, commands[i].label) == 0) {
			/*int return_code =*/ commands[i].func();
			return;
		}
	}
	
	if(arg_idx > 0)
		printf("shell: %s: command not found\n", cmd);
	
	return;
}

void shell_callback(int code) {
	extern Device* device_stdout;
	stdio_scancode(code);
	
	int ch = stdio_getchar();
	while(ch >= 0) {
		switch(ch) {
			case '\n':
				putchar(ch);
				exec();
				cmd_idx = 0;
				printf("# ");
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
}
