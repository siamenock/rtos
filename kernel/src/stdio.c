#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <byteswap.h>
#include <util/ring.h>
#include "device.h"
#include "port.h"
#include "mp.h"
#include "driver/vga.h"
#include "driver/keyboard.h"
#include "driver/stdin.h"
#include "driver/stdout.h"
#include "event.h"
#include "cpu.h"
#include "stdio.h"

#define BUFFER_SIZE	2048

struct _IO_FILE* stdin;
struct _IO_FILE* stdout;
struct _IO_FILE* stderr;

char __stdin[32];
size_t __stdin_head;
size_t __stdin_tail;
size_t __stdin_size = 32;

char __stdout[BUFFER_SIZE];
size_t __stdout_head;
size_t __stdout_tail;
size_t __stdout_size = BUFFER_SIZE;

char __stderr[BUFFER_SIZE];
size_t __stderr_head;
size_t __stderr_tail;
size_t __stderr_size = BUFFER_SIZE;

Device* device_stdin;
Device* device_stdout;
Device* device_stderr;

#define write1(_buf, _len) ((CharOut*)device_stdout->driver)->write(device_stdout->id, _buf, _len)
#define write2(_buf, _len) ((CharOut*)device_stderr->driver)->write(device_stderr->id, _buf, _len)
#define set_cursor1(_rows, _cols) ((CharOut*)device_stdout->driver)->set_cursor(device_stdout->id, _rows, _cols)
#define get_cursor1(_rows, _cols) ((CharOut*)device_stdout->driver)->get_cursor(device_stdout->id, _rows, _cols)

void stdio_init() {
	if(mp_core_id() == 0) {
		// Init Standard I/O/E
		device_stdin = device_register(keyboard_type, &keyboard_driver, NULL, NULL);
		device_stderr = device_stdout = device_register(vga_type, &vga_driver, NULL, NULL);
	} else {
		device_stdin = device_register(stdin_type, &stdin_driver, NULL, NULL);
		device_stdout = device_register(stdout_type, &stdout_driver, NULL, "stdout");
		device_stderr = device_register(stdout_type, &stdout_driver, NULL, "stderr");
	}
}

void stdio_init2(void* buf, size_t size) {
	((CharOut*)device_stdout->driver)->set_buffer(device_stdout->id, buf, size);
}

static void stdio_dump_ring(char* header, char* buffer, size_t* head, size_t tail, size_t size) {
	int header_len = strlen(header);
	int body_len = 80 - header_len;
	
	char* strchrn(const char* s, const char* e, int c) {
		char* ch = (char*)s;
		
		while(*ch != c && *ch != '\0' && ch < e)
			ch++;
		
		return ch < e && *ch == c ? ch: NULL;
	}
	
	char* dump_lines(char* h, char* e) {
		char* t = strchrn(h, e, '\n');
		while(t) {
			t++;
			while(t - h > body_len) {
				write1(header, header_len);
				write1(h, body_len);
				
				h += body_len;
			}
			
			write1(header, header_len);
			write1(h, t - h);
			
			if(t >= e)
				return NULL;
			
			h = t;
			t = strchrn(h, e, '\n');
		}
		
		while(e - h > body_len) {
			write1(header, header_len);
			write1(h, body_len);
			
			h += body_len;
		}
		
		return h < e ? h : NULL;
	}
	
	char* h = buffer + *head;
	char* e = buffer + tail;
	if(*head > tail) {
		h = dump_lines(h, buffer + size);
		if(h) {
			int len1 = buffer + size - h;
			write1(header, header_len);
			write1(h, len1);
			
			int len2 = body_len - len1;
			if(len2 > tail)
				len2 = tail;
			
			char* t = strchrn(h, buffer + len2, '\n');
			if(t) {
				t++;
				write1(h, t - h);
				h = t;
			} else {
				write1(buffer, len2);
				write1("\n", 1);
				h = buffer + len2;
			}
		} else {
			h = buffer;
		}
	}
	
	h = dump_lines(h, e);
	if(h) {
		write1(header, header_len);
		write1(h, e - h);
		write1("\n", 1);
	}
	
	*head = tail;
}

void stdio_event(void* data) {
	#define HEX(v)	(((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
	
	char header[10] = "Core 01> ";
	int count = mp_core_count();
	
	for(int i = 1; i < count; i++) {
		char* buffer = (char*)MP_CORE(__stdout, i);
		size_t* head = (size_t*)MP_CORE(&__stdout_head, i);
		size_t tail = *(size_t*)MP_CORE(&__stdout_tail, i);
		size_t size = *(size_t*)MP_CORE(&__stdout_size, i);
		
		if(*head != tail) {
			header[5] = HEX(i >> 4);
			header[6] = HEX(i >> 0);
			header[7] = '>';
			
			stdio_dump_ring(header, buffer, head, tail, size);
		}
		
		buffer = (char*)MP_CORE(__stderr, i);
		head = (size_t*)MP_CORE(&__stderr_head, i);
		tail = *(size_t*)MP_CORE(&__stderr_tail, i);
		size = *(size_t*)MP_CORE(&__stderr_size, i);
		
		if(*head != tail) {
			header[5] = HEX(i >> 4);
			header[6] = HEX(i >> 0);
			header[7] = '!';
			
			stdio_dump_ring(header, buffer, head, tail, size);
		}
	}
}

// Ref: http://www.powerindex.net/U_convt/ascii/ascii.htm
static const char mapping[][2] = {
	{ 0, 0 },	// 0x00
	{ 0x1b, 0x1b },
	{ '1', '!' },
	{ '2', '@' },
	{ '3', '#' },
	{ '4', '$' },
	{ '5', '%' },
	{ '6', '^' },
	{ '7', '&' },
	{ '8', '*' },
	{ '9', '(' },
	{ '0', ')' },
	{ '-', '_' },
	{ '=', '+' },
	{ '\b', '\b' },	// Back space
	
	{ '\t', '\t' },	// Tap
	{ 'q', 'Q' },	// Scan code: 0x10
	{ 'w', 'W' },
	{ 'e', 'E' },
	{ 'r', 'R' },
	{ 't', 'T' },
	{ 'y', 'Y' },
	{ 'u', 'U' },
	{ 'i', 'I' },
	{ 'o', 'O' },
	{ 'p', 'P' },
	{ '[', '{' },
	{ ']', '}' },
	{ '\n', '\n' },	// Carriage Return
	
	{ 0, 0 },	// Left ctrl
	{ 'a', 'A' },
	{ 's', 'S' },
	{ 'd', 'D' },	// Scan code: 0x20
	{ 'f', 'F' },
	{ 'g', 'G' },
	{ 'h', 'H' },
	{ 'j', 'J' },
	{ 'k', 'K' },
	{ 'l', 'L' },
	{ ';', ':' },
	{ '\'', '"' },
	{ '`', '~' },
	
	{ 0, 0 },	// Left shift
	{ '\\', '|' },
	{ 'z', 'Z' },
	{ 'x', 'X' },
	{ 'c', 'C' },
	{ 'v', 'V' },
	{ 'b', 'B' },	// Scan code: 0x30
	{ 'n', 'N' },
	{ 'm', 'M' },
	{ ',', '<' },
	{ '.', '>' },
	{ '/', '?' },
	{ 0, 0 },	// Right shift
	{ '*', '*' },
	
	{ 0, 0 },	// Left alt
	{ ' ', ' ' },
	{ 0, 0 },	// Caps lock
	{ 0, 0 },	// F1
	{ 0, 0 },	// F2
	{ 0, 0 },	// F3
	{ 0, 0 },	// F4
	{ 0, 0 },	// F5
	{ 0, 0 },	// F6, Scan code: 0x40
	{ 0, 0 },	// F7
	{ 0, 0 },	// F8
	{ 0, 0 },	// F9
	{ 0, 0 },	// F10
	
	{ 0, 0 },	// Num lock
	{ 0, 0 },	// Scroll lock
	{ 0, '7' },	// Home
	{ 0, '8' },	// Up
	{ 0, '9' },	// Page Up
	{ '-', '-'},
	{ 0, '4' },	// Left
	{ 0, '5' },	// Center
	{ 0, '6' },	// Right
	{ '+', '+' },
	{ 0, '1' },	// End
	{ 0, '2' },	// Down, Scan code: 0x50
	{ 0, '3' },	// Page down
	{ 0, '0' },	// Insert
	{ 0x7f, '.' },	// Delete
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },	// F11
	{ 0, 0 },	// F12
	{ 0, 0 },	// 
	{ 0, 0 },	// Scan code: 0x60
};

static int shift;
static int ctrl;
static int alt;
static bool capslock;
static bool numlock;
static bool scrolllock;
static int extention;

#if 0
static int print_string2(const char* str, int len) {
	char buf[64];
	
	int i, s;
	while(len > 0) {
		s = len < 64 ? len : 64;
		for(i = 0; i < s; i++) {
			buf[i] = *str++;
		}
		
		write1((void*)buf, s);
		
		len -= s;
	}
	
	return len;
}

static int print_char(char ch) {
	write1((void*)&ch, 1);
	
	return 1;
}

static int print_string(char* str) {
	int i;
	for(i = 0; str[i] != 0; i++);
	
	write1((void*)str, i);

	return i;
}

static int print_integer(int value, int base, char a, int width, char fill) {
	char buf[64];
	
	int sign = value < 0;
	if(sign)
		value *= -1;
	
	int i = 63;
	while(value != 0) {
		int mod = value % base;
		if(mod > 9)
			buf[i--] = a + mod - 10;
		else
			buf[i--] = '0' + mod;
		value /= base;
	}
	
	if(sign) {
		buf[i] = '-';
	} else if(i == 63) {
		buf[i] = '0';
	} else {
		i++;
	}
	
	while(64 - i < width) {
		buf[--i] = fill;
	}
	
	write1((void*)(buf + i), 64 - i);

	return 64 - i;
}

static int print_unsigned_integer(unsigned int value, int base, char a, int width, char fill) {
	char buf[64];
	
	int sign = value < 0;
	if(sign)
		value *= -1;
	
	int i = 63;
	while(value != 0) {
		int mod = value % base;
		if(mod > 9)
			buf[i--] = a + mod - 10;
		else
			buf[i--] = '0' + mod;
		value /= base;
	}
	
	if(sign) {
		buf[i] = '-';
	} else if(i == 63) {
		buf[i] = '0';
	} else {
		i++;
	}
	
	while(64 - i < width) {
		buf[--i] = fill;
	}
	
	if(width > 0 && 64 - i > width)
		i = 64 - width;
	
	write1((void*)(buf + i), 64 - i);

	return 64 - i;
}

static int print_long(long value, int base, char a, int width, char fill) {
	char buf[64];
	
	int sign = value < 0;
	if(sign)
		value *= -1;
	
	int i = 63;
	while(value != 0) {
		int mod = value % base;
		if(mod > 9)
			buf[i--] = a + mod - 10;
		else
			buf[i--] = '0' + mod;
		value /= base;
	}
	
	if(sign) {
		buf[i] = '-';
	} else if(i == 63) {
		buf[i] = '0';
	} else {
		i++;
	}
	
	while(64 - i < width) {
		buf[--i] = fill;
	}
	
	write1((void*)(buf + i), 64 - i);

	return 64 - i;
}

static int print_unsigned_long(unsigned long value, int base, char a, int width, char fill) {
	char buf[64];
	
	int sign = value < 0;
	if(sign)
		value *= -1;
	
	int i = 63;
	while(value != 0) {
		int mod = value % base;
		if(mod > 9)
			buf[i--] = a + mod - 10;
		else
			buf[i--] = '0' + mod;
		value /= base;
	}
	
	if(sign) {
		buf[i] = '-';
	} else if(i == 63) {
		buf[i] = '0';
	} else {
		i++;
	}
	
	while(64 - i < width) {
		buf[--i] = fill;
	}
	
	if(width > 0 && 64 - i > width)
		i = 64 - width;
	
	write1((void*)(buf + i), 64 - i);
	
	return 64 - i;
}

static int print_double(double v) {
	char buf[64];
	
	long l = (long)v;
	print_long(l, 10, 0, 0, ' ');
	
	int i = 0;
	buf[i] = '.';
	
	for(i = 1 ; i < 8; i++) {
		v = (v - l) * 10;
		if(v == 0.0)
			break;
		
		l = (long)v;
		buf[i] = (char)l + '0';
	}
	
	while(i >= 1 && buf[i] == '0')
		i--;
	
	if(i == 1)
		buf[i++] = '0';
	
	write1((void*)(buf), i);
	
	return i;
}
#endif

int _IO_putc(int ch, struct _IO_FILE* fp) {
	uint8_t c = ch;
	if(fp == stdout) {
		return write1((void*)&c, 1);
	} else if(fp == stderr) {
		write2((void*)&c, 1);
	}
	
	return -1;
}

static void led() {
	port_out8(0x60, 
		(((uint8_t)capslock) << 2) |
		(((uint8_t)numlock) << 1) |
		(((uint8_t)scrolllock) << 0));
}

void stdio_scancode(int code) {
	// Parse control code
	if(extention == 0) {
		switch(code) {
			case 0x1d:		// left ctrl press
				ctrl++;
				return;
			case 0x2a:		// left shift press
				shift++;
				return;
			case 0x36: 		// right shift press
				shift++;
				return;
			case 0x38:		// left alt press
				alt++;
				return;
			case 0x3a:
				capslock = !capslock;
				led();
				return;
			case 0x45:
				numlock = !numlock;
				led();
				return;
			case 0x46:
				scrolllock = !scrolllock;
				led();
				return;
			case 0x9d:		// ctrl release
				ctrl--;
				return;
			case 0xaa:		// left shift release
				shift--;
				return;
			case 0xb6: 		// right shift release
				shift--;
				return;
			case 0xb8:		// alt release
				alt--;
				return;
			case 0xe0:
				extention = 1;
				return;
			case 0xe1:
				extention = 2;
				return;
		}
	}
	
	// Parse char scancode
	#define PUT()		ring_write(__stdin, __stdin_head, &__stdin_tail, __stdin_size, &ch, 1)
	
	bool is_cap = false;
	char ch;
	if(extention > 0) {
		switch(code) {
			case 0x47:	// home
				ch = 0x1b;	// ESC
				PUT();
				ch = 'O';
				PUT();
				ch = 'H';
				PUT();
				break;
			case 0x49:	// page up
				ch = 0x1b;	// ESC
				PUT();
				ch = '[';
				PUT();
				ch = 0x35;
				PUT();
				ch = '~';
				PUT();
				break;
			case 0x4f:	// end
				ch = 0x1b;	// ESC
				PUT();
				ch = 'O';
				PUT();
				ch = 'F';
				PUT();
				break;
			case 0x51:	// page down
				ch = 0x1b;	// ESC
				PUT();
				ch = '[';
				PUT();
				ch = 0x36;
				PUT();
				ch = '~';
				PUT();
				break;
		}
		
		extention--;
	} else if(code <= 0x60) {
		ch = mapping[code][0];
		if(code <= 0x58 && ch == 0)
			return;
		
		if(ch >= 'a' && ch <= 'z') {
			if(shift > 0)
				is_cap = !is_cap;
			
			if(capslock)
				is_cap = !is_cap;
			
			if(is_cap)
				ch = mapping[code][1];
		} else {
			if(code >= 0x47) {
				if(numlock)
					ch = mapping[code][1];
			} else if(shift > 0) {
				ch = mapping[code][1];
			}
		}
		
		if(ctrl > 0) {
			if(ch >= 0x40 && ch < 0x60)
				ch -= 0x40;
			else if(ch >= 0x60 && ch < 0x80)
				ch -= 0x60;
		}
		
		PUT();
	}
	
	// TODO: make numlock mapping table
}

int stdio_getchar() {
	char ch;
	if(ring_read(__stdin, &__stdin_head, __stdin_tail, __stdin_size, &ch, 1) > 0)
		return (int)ch;
	else
		return -1;
}

/*
void _print(const char* str, int row, int col) {
	char* v = (char*)0xb8000 + (row * 160) + col * 2;
	while(*str != 0) {
		*(v++) = *str;
		*(v++) = 0x07;
		str++;
	}
}
*/

int vsprintf(char *str, const char *format, va_list va) {
	char* str0 = str;
	
	char fill;
	int width;
	int width2;
	
	void print_string(char* str2) {
		while(*str2 != 0) {
			*str++ = *str2++;
		}
	}
	
	void print_integer(int value, char a, int base) {
		if(value < 0) {
			*str++ = '-';
			value = -value;
		}
		
		char* str0 = str;
		do {
			int mod = value % base;
			if(mod >= 10)
				*str++ = a + mod - 10;
			else
				*str++ = '0' + mod;
			
			value /= base;
		} while(value != 0);
		
		while(str - str0 < width) {
			*str++ = fill;
		}
		
		int half = (str - str0) / 2;
		for(int i = 0; i < half; i++) {
			char tmp = str0[i];
			str0[i] = str[-i - 1];
			str[-i - 1] = tmp;
		}
	}
	
	void print_long(long value, char a, int base) {
		if(value < 0) {
			*str++ = '-';
			value = -value;
		}
		
		char* str0 = str;
		do {
			int mod = value % base;
			if(mod >= 10)
				*str++ = a + mod - 10;
			else
				*str++ = '0' + mod;
			
			value /= base;
		} while(value != 0);
		
		while(str - str0 < width) {
			*str++ = fill;
		}
		
		int half = (str - str0) / 2;
		for(int i = 0; i < half; i++) {
			char tmp = str0[i];
			str0[i] = str[-i - 1];
			str[-i - 1] = tmp;
		}
	}
	
	void print_unsigned_integer(unsigned int value, char a, int base) {
		char* str0 = str;
		do {
			int mod = value % base;
			if(mod >= 10)
				*str++ = a + mod - 10;
			else
				*str++ = '0' + mod;
			
			value /= base;
		} while(value != 0);
		
		while(str - str0 < width) {
			*str++ = fill;
		}
		
		int half = (str - str0) / 2;
		for(int i = 0; i < half; i++) {
			char tmp = str0[i];
			str0[i] = str[-i - 1];
			str[-i - 1] = tmp;
		}
	}
	
	void print_unsigned_long(unsigned long value, char a, int base) {
		char* str0 = str;
		do {
			int mod = value % base;
			if(mod >= 10)
				*str++ = a + mod - 10;
			else
				*str++ = '0' + mod;
			
			value /= base;
		} while(value != 0);
		
		while(str - str0 < width) {
			*str++ = fill;
		}
		
		int half = (str - str0) / 2;
		for(int i = 0; i < half; i++) {
			char tmp = str0[i];
			str0[i] = str[-i - 1];
			str[-i - 1] = tmp;
		}
	}
	
	void print_double(double value) {
		long l = (long)value;
		print_long(l, 0, 10);
		
		*str++ = '.';
		
		if(width2 == 0)
			width2 = 6;
		
		for(int i = 0; i < width2; i++) {
			value = (value - l) * 10;
			if(value == 0.0)
				break;
			
			l = (long)value;
			*str++ = (char)l + '0';
		}
	}
	
	while(1) {
		switch(*format) {
			case '%':
				format++;
				fill = ' ';
				width = 0;
				width2 = 0;
				if(*format >= '0' && *format <= '9') {
					if(*format == '0') {
						fill = '0';
					}
					
					while(*format >= '0' && *format <= '9') {
						width = width * 10 + *format++ - '0';
					}
				} else if(*format == '#') {
					format++;
					switch(*format) {
						case 'x':
							*str++ = '0';
							*str++ = 'x';
							break;
						case 'o':
							*str++ = '0';
							break;
					}
				}
				
				if(*format == '.') {
					format++;
					
					while(*format >= '0' && *format <= '9') {
						width2 = width2 * 10 + *format++ - '0';
					}
				}
				
				switch(*format) {
					case '%':
						*str++ = *format++;
						break;
					case 'c':
						*str++ = va_arg(va, int);
						format++;
						break;
					case 's':
						print_string(va_arg(va, char*));
						format++;
						break;
					case 'd':
					case 'i':
						print_integer(va_arg(va, int), 0, 10);
						format++;
						break;
					case 'u':
						print_unsigned_integer(va_arg(va, unsigned int), 0, 10);
						format++;
						break;
					case 'l':
						format++;
						switch(*format) {
							case 'u':
								print_unsigned_long(va_arg(va, unsigned long), 0, 10);
								format++;
								break;
							case 'd':
								print_long(va_arg(va, unsigned long), 0, 10);
								format++;
								break;
							case 'x':
								print_unsigned_long(va_arg(va, unsigned long), 'a', 16);
								format++;
								break;
							case 'X':
								print_unsigned_long(va_arg(va, unsigned long), 'A', 16);
								format++;
								break;
							case 'o':
								print_unsigned_long(va_arg(va, unsigned long), 0, 8);
								format++;
								break;
							case 'O':
								print_unsigned_long(va_arg(va, unsigned long), 0, 8);
								format++;
								break;
							default:
								*str++ = 0;
								goto done;
						}
						break;
					case 'p':
						width = 16;
						fill = '0';
						print_unsigned_long(va_arg(va, unsigned long), 'a', 16);
						format++;
						break;
					case 'x':
						print_unsigned_integer(va_arg(va, unsigned int), 'a', 16);
						format++;
						break;
					case 'X':
						print_unsigned_integer(va_arg(va, unsigned int), 'A', 16);
						format++;
						break;
					case 'o':
						print_unsigned_integer(va_arg(va, unsigned int), 0, 8);
						format++;
						break;
					case 'O':
						print_unsigned_integer(va_arg(va, unsigned int), 0, 8);
						format++;
						break;
					case 'f':
						print_double(va_arg(va, double));
						format++;
						break;
					default:
						*str++ = 0;
						// Unknown format
						goto done;
				}
				
				break;
			case 0:
				*str++ = *format++;
				goto done;
			default:
				*str++ = *format++;
		}
	}
done:
	
	return str - str0;
}

int sprintf(char *str, const char *format, ...) {
	va_list va;
	va_start(va, format);
	int len = vsprintf(str, format, va);
	va_end(va);
	
	return len;
}

int printf(const char* format, ...) {
	char buf[4096];
	
	va_list va;
	va_start(va, format);
	int len = vsprintf(buf, format, va);
	va_end(va);
	
	return write1(buf, len);
}

int fprintf(FILE* stream, const char* format, ...) {
	char buf[4096];
	
	va_list va;
	va_start(va, format);
	int len = vsprintf(buf, format, va);
	va_end(va);
	
	return write1(buf, len);
}

void exit(int status) {
	printf("Exit is called: %d\n", status);
	while(1) asm("hlt");
}

clock_t clock() {
       return (clock_t)(cpu_tsc() / (cpu_frequency / CLOCKS_PER_SEC));
}

int putchar(int c) {
	char ch = c;
	return write1(&ch, 1);
}

uint32_t htonl(uint32_t hostlong) {
	return bswap_32(hostlong);
}

uint16_t htons(uint16_t hostshort) {
	return bswap_16(hostshort);
}

uint32_t ntohl(uint32_t netlong) {
	return bswap_32(netlong);
}

uint16_t ntohs(uint16_t netshort) {
	return bswap_16(netshort);
}

