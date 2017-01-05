#include <stdbool.h>
#include <string.h>
#include "../port.h"
#include "vga.h"

#define REG_ADDR		0x03d4
#define REG_DATA		0x03d5
#define CURSOR_UPPER		0x0e
#define CURSOR_LOWER		0x0f
#define DEFAULT_ATTRIBUTE	0x07

#define COLS			80
#define ROWS			25
#define LINE			(COLS * 2)
#define SCREEN			(LINE * ROWS)

static CharOutInit init_data;
static char* buffer;
static int head;
static int tail;
static int size = ROWS;
static int screen;
static int row;
static int col;
static uint8_t attr = DEFAULT_ATTRIBUTE;
	
static void cursor_hide() {
	int index = -1;
	
	port_out8(REG_ADDR, CURSOR_UPPER);
	port_out8(REG_DATA, index >> 8);
	
	port_out8(REG_ADDR, CURSOR_LOWER);
	port_out8(REG_DATA, index & 0xff);
}

static void cursor() {
	int index = row * 80 + col;
	
	port_out8(REG_ADDR, CURSOR_UPPER);
	port_out8(REG_DATA, index >> 8);
	
	port_out8(REG_ADDR, CURSOR_LOWER);
	port_out8(REG_DATA, index & 0xff);
}

const uint64_t EMPTY = 0x0700070007000700L;

static void clean(int line, int lines) {
	uint64_t* p = (void*)(buffer + line * LINE);
	for(int i = 0; i < 20 * lines; i++) {
		*p++ = EMPTY;
	}
}

static void extend(int lines) {
	if(lines > size) {
		screen = head = 0;
		tail = ROWS;
		clean(0, ROWS);
	} else if(head < tail) {
		if(tail + lines <= size) {
			clean(tail, lines);
			tail += lines;
		} else {
			clean(tail, size - tail);
			tail = lines - (size - tail);
			clean(0, tail);
			
			if(tail > head)
				head = tail;
		}
	} else {
		if(tail + lines <= size) {
			clean(tail, lines);
			tail += lines;
			
			if(tail > head)
				head = tail;
		} else {
			clean(tail, size - tail);
			tail = lines - (size - tail);
			clean(0, tail);
			
			head = tail;
		}
	}
}

static void render() {
	if(init_data.is_render) {
		if(screen + ROWS >= size) {
			int len1 = (size - screen) * LINE;
			int len2 = SCREEN - len1;
			
			memcpy((char*)0xb8000, buffer + screen * LINE, len1);
			memcpy((char*)0xb8000 + len1, buffer, len2);
		} else {
			memcpy((char*)0xb8000, buffer + screen * LINE, SCREEN);
		}
	}
}

static bool is_end() {
	int s = tail - ROWS;
	if(s < 0)
		s += size;
	
	return s == screen;
}

void scroll_up(int lines) {
	if(screen >= head) {
		if(screen - lines < head) {
			screen = head;
		} else {
			screen -= lines;
		}
	} else {
		if(screen + size - head < lines) {
			screen = head;
		} else {
			screen -= lines;
			
			if(screen < 0)
				screen += size;
		}
	}
	
	if(is_end())
		cursor();
	else
		cursor_hide();
}

static void home() {
	screen = head;
	
	if(is_end())
		cursor();
	else
		cursor_hide();
}

static void scroll_down(int lines) {
	if(is_end())
		return;
	
	if(screen < tail) {
		screen += lines;
		
		if(screen + ROWS >= tail)
			screen = tail - ROWS;
	} else {
		screen += lines;
		
		if(screen + ROWS >= size && screen + ROWS - size >= tail)
			screen = tail - ROWS;
		
		if(screen >= size)
			screen -= size;
	}
	
	if(screen < 0)
		screen += size;
	
	if(is_end())
		cursor();
	else
		cursor_hide();
}

static void end() {
	screen = tail - ROWS;
	
	if(screen < 0)
		screen += size;
	
	cursor();
}

/*
static ssize_t vga_read(Device* device, void* buf, size_t size) {
	size = size > SCREEN ? SCREEN : size;
	size -= size % 2;
	
	memcpy(buf, (char*)0xb8000, size);
	
	return size;
}
*/

static int init(void* device, void* data) {
	if(data == NULL)
		return -1;
	
	memcpy(&init_data, data, sizeof(CharOutInit));
	
	buffer = init_data.buf;
	screen = 0;
	head = 0;
	tail = ROWS;
	size = init_data.len / LINE;
	
	if(init_data.is_capture) {
		memcpy(buffer, (char*)0xb8000, 80 * 2 * 25);
		
		uint16_t pos = 0;
		
		port_out8(REG_ADDR, CURSOR_UPPER);
		pos |= (uint16_t)port_in8(REG_DATA) << 8;
		
		port_out8(REG_ADDR, CURSOR_LOWER);
		pos |= port_in8(REG_DATA);
		
		row = pos / COLS;
		col = pos % COLS;
	}
	
	return 0;
}

static void destroy(int id) {
	// Do nothing
}

static int write(int id, const char* buf, int len) {
	const char* ch = buf;
	
	if(!is_end()) {
		end();
	}
	
	while(len-- > 0) {
		char* v = (char*)0xb8000 + row * LINE + col * 2;
		char* s = screen + row < size ? 
			buffer + (screen + row) * LINE + col * 2:
			buffer + (screen + row - size) * LINE + col * 2;
		
		switch(*ch) {
			case 0:
				break;
			case 7:
				// TODO: Beep
				break;
			case '\b':
				if(row != 0 || col != 0) {
					s[-2] = 0;
					s[-1] = attr;
					
					if(init_data.is_render) {
						v[-2] = 0;
						v[-1] = attr;
					}
					
					col--;
					
					if(col < 0) {
						col += 80;
						row--;
					}
				}
				break;
			case '\r':
				col = 0;
				break;
			case '\f':
				extend(row);
				end();
				row = 0;
				render();
				break;
			case '\t':
				col += 8 - (col % 8);
				break;
			case '\n':
				row++;
				col = 0;
				if(row >= ROWS) {
					extend(1);
					screen = (screen + 1) % size;
					render();
					row--;
				}
				break;
			case 0x1b:	// ESC code
				;
				ch++;
				uint32_t code = 0;
				while(*ch >= '0' && *ch <= '9') {
					code *= 10;
					code += *ch - '0';
					ch++;
				}
				
				switch(*ch) {
					case 'm':
						switch(code) {
							case 0:
								attr = DEFAULT_ATTRIBUTE;
								break;
							case 30:
								attr = (attr & 0xf0) | 0x00;
								break;
							case 31:
								attr = (attr & 0xf0) | 0x04;
								break;
							case 32:
								attr = (attr & 0xf0) | 0x02;
								break;
							case 33:
								attr = (attr & 0xf0) | 0x06;
								break;
							case 34:
								attr = (attr & 0xf0) | 0x01;
								break;
							case 35:
								attr = (attr & 0xf0) | 0x05;
								break;
							case 36:
								attr = (attr & 0xf0) | 0x03;
								break;
							case 37:
								attr = (attr & 0xf0) | 0x07;
								break;
							case 38:
								// None
								break;
							case 39:
								attr = (attr & 0xf0) | (DEFAULT_ATTRIBUTE & 0x0f);
								break;
							case 40:
								attr = (attr & 0x0f) | 0x00;
								break;
							case 41:
								attr = (attr & 0x0f) | 0x40;
								break;
							case 42:
								attr = (attr & 0x0f) | 0x20;
								break;
							case 43:
								attr = (attr & 0x0f) | 0x60;
								break;
							case 44:
								attr = (attr & 0x0f) | 0x10;
								break;
							case 45:
								attr = (attr & 0x0f) | 0x50;
								break;
							case 46:
								attr = (attr & 0x0f) | 0x30;
								break;
							case 47:
								attr = (attr & 0x0f) | 0x70;
								break;
							case 48:
								// None
								break;
							case 49:
								attr = (attr & 0x0f) | (DEFAULT_ATTRIBUTE & 0xf0);
								break;
							default:
								;
						}
						break;
					default:
						; // Unknown ESC code
				}
				break;
			default:
				s[0] = *ch;
				s[1] = attr;
				
				if(init_data.is_render) {
					v[0] = *ch;
					v[1] = attr;
				}
				col++;
		}
		ch++;
		
		if(col >= 80) {
			col -= 80;
			extend(1);
			screen = (screen + 1) % size;
			render();
		}
		
		cursor();
	}
	
	return len;
}

static int scroll(int id, int lines) {
	if(lines == 0) {
		// Do nothing, just refresh
	} else if(lines <= -size) {
		home();
		lines = -size;
	} else if(lines >= size) {
		end();
		lines = size;
	} else if(lines > 0) {
		scroll_down(lines);
	} else {
		scroll_up(-lines);
	}
	
	render();
	
	return lines;
}

static void set_cursor(int id, int rows, int cols) {
	row = rows;
	col = cols;
	cursor();
}

static void get_cursor(int id, int* rows, int* cols) {
	*rows = row;
	*cols = col;
}

static void set_render(int id, int is_render) {
	init_data.is_render = is_render;
}

static int is_render(int id) {
	return init_data.is_render;
}

DeviceType vga_type = DEVICE_TYPE_CHAR_OUT;

CharOut vga_driver = {
	.init = init,
	.destroy = destroy,
	.write = write,
	.scroll = scroll,
	.set_cursor = set_cursor,
	.get_cursor = get_cursor,
	.set_render = set_render,
	.is_render = is_render
};
