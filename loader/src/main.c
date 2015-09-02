#include "string.h"
#include "page.h"
#include "mode_switch.h"
#include "pnkc.h"
#include "port.h"
#include "tlsf.h"
#include "time.h"
#include "driver/fs.h"
#include "driver/bfs.h"
#include "driver/pata.h"
#include "driver/usb/usb.h"

#define DEBUG	0

#define NORMAL	0x07
#define PASS	0x02
#define FAIL	(0x04 | 0x08)

static void print(int x, int y, const char* str, char attr);
static void print_32(int x, int y, uint32_t v, char attr);
static void print_64(int x, int y, uint64_t v, char attr);
static void print_ptr(int x, int y, void* ptr, char attr);
static void clean(volatile uint32_t* addr, uint32_t size);
static uint8_t get_core_id();
static int check_memory_size();
static void malloc_init();
static void get_cpu_vendor_string(char* str);
static int check_x86_64_support();
static int find_kernel(PNKC* pnkc);
static int copy_kernel(PNKC* pnkc);
static int copy_kernel_half(uint8_t core_id);
static void dump(int x, int y, void* addr);

void main(void) {
	uint8_t core_id = get_core_id();;
	
	// BSP routine
	if(core_id == 0) {
		print(0, 1, "Switched to protected mode...", NORMAL);
		
		print(0, 2, "Checking memory size", NORMAL);
		if(check_memory_size()) {
			print(50, 2, "Pass", PASS);
		} else {
			print(50, 2, "Fail", FAIL);
			while(1) asm("hlt");
		}
	
		print(0, 3, "Getting CPU vendor string", NORMAL);
		char vendor[13] = { 0, };
		get_cpu_vendor_string(vendor);
		print(50, 3, vendor, PASS);

		print(0, 4, "Initializing malloc memory", NORMAL);
		malloc_init();
		print(50, 4, "Pass", PASS);
	
		print(0, 5, "Checking x86_64 supported", NORMAL);
		if(check_x86_64_support()) {
			print(50, 5, "Pass", PASS);
		} else {
			print(50, 5, "Fail", FAIL);
			while(1) asm("hlt");
		}

		print(0, 6, "Measuring CPU clock", NORMAL);
		time_init();
		print(50, 6, "Pass", PASS);

		#ifdef DEBUG
		/** 
		 * VGA driver
		 */
		console_init();
		#endif

		print(0, 7, "Initializing USB controller driver", NORMAL);
		usb_initialize();
		print(50, 7, "Pass", PASS);

		print(0, 8, "Initializing disk drivers", NORMAL);
		disk_init0();
		disk_register(&pata_driver);
		disk_register(&usb_msc_driver);
	
		if(disk_init()) {
			print(50, 8, "Pass", PASS);
		} else {
			print(50, 8, "Fail", FAIL);
			while(1) asm("hlt");
		}

		print(0, 9, "Initializing file system", NORMAL);
		if(!bfs_init()) {
			print(50, 9, "Fail BFS", FAIL);
			while(1) asm("hlt");
		}

		if(!fs_init()) {
			print(50, 9, "Fail FS", FAIL);
			while(1) asm("hlt");
		} else {
			print(50, 9, "Pass", PASS);
		}

		print(0, 10, "Copying kernel", NORMAL);
		PNKC pnkc;
		if(!find_kernel(&pnkc)) {
			print(50, 10, "Fail", FAIL);
			while(1) asm("hlt");
		}

		if(copy_kernel(&pnkc)) {
			print(50, 10, "Pass", PASS);
		} else {
			print(50, 10, "Fail", FAIL);
			while(1) asm("hlt");
		}

		print(0, 11, "Initialize memory page tables", NORMAL);
		if(init_page_tables(core_id)) {
			print(50, 10, "Pass", PASS);
		} else {
			print(50, 10, "Fail", FAIL);
			while(1) asm("hlt");
		}

		print(0, 12, "Switching to 64 bit mode...", NORMAL);
		asm("nop" : : "d"((uint32_t)core_id));

		mode_switch();
	} else { // AP routine
		copy_kernel_half(core_id);
		init_page_tables(core_id);
		
		asm("nop" : : "d"((uint32_t)core_id));

		mode_switch();
	}
}

uint8_t get_core_id() {
	void cpuid(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
		asm volatile("cpuid"
			: "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
			: "a"(*a), "b"(*b), "c"(*c), "d"(*d));
	}

	bool check_extended_topology() {
		uint32_t a, b, c, d;
		a = 0x0;
		cpuid(&a, &b, &c, &d);

		if(a >= 11) {
			a = 0x0b;
			c = 0x0;
			cpuid(&a, &b, &c, &d);

			if(b)
				return true;
			else
				return false;
		} else
			return false;
	}

	uint32_t a, b, c, d;
	if(check_extended_topology()) {
		a = 0x0b;
		c = 0x0;
		cpuid(&a, &b, &c, &d);

		if((b & 0xffff) == 1) {
			return ((d & 0xff) >> (a & 0xf));
		} else {
			return d & 0xff;
		}
	} else {
		a = 0x01;
		cpuid(&a, &b, &c, &d);

		return (b >> 24) & 0xff;
	}
}

int check_memory_size() {
	volatile uint32_t* addr = (uint32_t*)0x100000;	// 1MB
	while(addr < (uint32_t*)0x4000000) {	// 64MB
		uint32_t old = *addr;
		*addr = 0x12345678;
		if(*addr != 0x12345678)
			return 0;
		*addr = old;
		
		addr += 0x100000 / sizeof(uint32_t*);
	}
	
	return 1;
}

extern void* __malloc_pool;

static void malloc_init() {
	/**
	 * Heap size is 192kB. Loader starts at 0x10000 guranteed 256kB size
	 * Be aware of caculating loader size. Data + ROData + BSS. 
	 * Currently loader is 74kB, it affect 0x44ee8. 
	 */
	uintptr_t start = 0x50000;
	uintptr_t end = 0x80000; 

	__malloc_pool = (uintptr_t*)start;

	init_memory_pool((uint32_t)(end - start), __malloc_pool, 0);
}

void clean(volatile uint32_t* addr, uint32_t size) {
	volatile uint32_t* end = addr + size / 4;
	while(addr < end)
		*addr++ = 0;
}

void get_cpu_vendor_string(char* str) {
	uint32_t* s = (uint32_t*)str;
	
	uint32_t a, b, c, d;
	asm("cpuid" 
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d) 
		: "a"(0));
	s[0] = b;
	s[1] = d;
	s[2] = c;
}

int check_x86_64_support() {
	uint32_t a, b, c, d;
	asm("cpuid" 
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d) 
		: "a"(0x80000001));
	
	return d & (1 << 29);
}

int find_kernel(PNKC* pnkc) {
	int fd = open("/kernel.bin", "r"); 
	if(fd < 0) 
		return 0;
	
	int pnkc_size = sizeof(PNKC);

	// PNKC writted at entry.bin
	if(read(fd, pnkc, pnkc_size) == pnkc_size) {
		if(pnkc->magic == PNKC_MAGIC) {
			close(fd);

			return 1;
		}
	} 

	close(fd);

	return 0;
}

int copy_section(int fd, uint32_t location, uint32_t size) {
	int animate = 0;
	char animation[4] = { '-', '/', '|', '\\' };
	static const int x = 50, y = 10;
	char* video = (char*)0xb8000 + (y * 160) + x * 2;
	video[1] = NORMAL;
	uint32_t count = 0;
	
	void memcpy(uint32_t* dest, uint32_t* src, uint32_t size) {
		while(size > 0) {
			if(size >= 4) {
				*dest++ = *src++;
				size -= 4;
			} else {
				uint8_t* d = (uint8_t*)dest;
				uint8_t* s = (uint8_t*)src;
				while(size > 0) {
					*d++ = *s++;
					size--;
				}
			}
			
			if(count++ % 512 == 0) {
				video[0] = animation[animate++ % 4];
			}
		}
	}

	uint8_t buf[FS_SECTOR_SIZE];
	int len, total_len = 0;
	int offset = 0;
	int copy_size = FS_SECTOR_SIZE;

	while((len = read(fd, buf, copy_size)) > 0) {
		total_len += len;
		size -= copy_size;

		memcpy((uint32_t*)(location + offset), (uint32_t*)buf, copy_size);
		offset += FS_SECTOR_SIZE;

		if(size < FS_SECTOR_SIZE)
			copy_size = size;

		if(size == 0) 
			break;
	}

	return 1;
}

int copy_kernel(PNKC* pnkc) {
	int fd = open("/kernel.bin", "r");
	if(fd < 0) 
		return -1;

	if(!lseek(fd, sizeof(PNKC), FS_SEEK_SET)) 
		return -2;

	clean((uint32_t*)0x200000, 0x400000);

	// Copy .text
	if(!copy_section(fd, 0x200000 + pnkc->text_offset, pnkc->text_size)) 
		return 0;

	// Copy .rodata
	if(!copy_section(fd, 0x200000 + pnkc->rodata_offset, pnkc->rodata_size)) 
		return 0;
	
	// Copy .data
	if(!copy_section(fd, 0x400000 + pnkc->data_offset, pnkc->data_size)) 
		return 0;
	
	// .bss is already inited

	// Write PNKC
	memcpy((uint32_t*)(0x200200 /* Kernel entry end */ - sizeof(PNKC)), (uint32_t*)pnkc, sizeof(PNKC));

	close(fd);

	return 1;
}

int copy_kernel_half(uint8_t core_id) {
	clean((uint32_t*)(0x400000 + core_id * 0x200000), 0x200000);

	PNKC* pnkc = (PNKC*)(0x200200 - sizeof(PNKC));

	// Copy .data
	memcpy((void*)(0x400000 + core_id * 0x200000 + pnkc->data_offset),
			(void*)(0x400000 + pnkc->data_offset), pnkc->data_size);
	// .bss is already inited

	return 1;
}

void dump(int x, int y, void* addr) {
	uint8_t* a = addr;
	uint8_t* v = (uint8_t*)0xb8000 + (y * 160) + x * 2;

	int i;
	for(i = 0; i < 32; i++) {
		v[i * 4] = (a[i] & 0xf) > 9 ? (a[i] & 0xf) - 10 + 'a' : (a[i] & 0xf) + '0';
		v[i * 4 + 2] = ((a[i] >> 4) & 0xf) > 9 ? ((a[i] >> 4) & 0xf) - 10 + 'a' : ((a[i] >> 4) & 0xf) + '0';
	}
}

void print(int x, int y, const char* str, char attr) {
	char* video = (char*)0xb8000 + (y * 160) + x * 2;

	for(int i = 0; str[i] != 0; i++) {
		video[i * 2] = str[i];
		video[i * 2 + 1] = attr;
	}
}

void print_32(int x, int y, uint32_t v, char attr) {
	char* video = (char*)0xb8000 + (y * 160) + x * 2;
	
	#define HEX(v)  (((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
	*video++ = HEX(v >> 28);
	*video++ = attr;
	*video++ = HEX(v >> 24);
	*video++ = attr;
	*video++ = HEX(v >> 20);
	*video++ = attr;
	*video++ = HEX(v >> 16);
	*video++ = attr;
	*video++ = HEX(v >> 12);
	*video++ = attr;
	*video++ = HEX(v >> 8);
	*video++ = attr;
	*video++ = HEX(v >> 4);
	*video++ = attr;
	*video++ = HEX(v >> 0);
	*video++ = attr;
}

void print_64(int x, int y, uint64_t v, char attr) {
	char* video = (char*)0xb8000 + (y * 160) + x * 2;
	
	#define HEX(v)  (((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
	*video++ = HEX(v >> 60);
	*video++ = attr;
	*video++ = HEX(v >> 56);
	*video++ = attr;
	*video++ = HEX(v >> 52);
	*video++ = attr;
	*video++ = HEX(v >> 48);
	*video++ = attr;
	*video++ = HEX(v >> 44);
	*video++ = attr;
	*video++ = HEX(v >> 40);
	*video++ = attr;
	*video++ = HEX(v >> 36);
	*video++ = attr;
	*video++ = HEX(v >> 32);
	*video++ = attr;
	*video++ = HEX(v >> 28);
	*video++ = attr;
	*video++ = HEX(v >> 24);
	*video++ = attr;
	*video++ = HEX(v >> 20);
	*video++ = attr;
	*video++ = HEX(v >> 16);
	*video++ = attr;
	*video++ = HEX(v >> 12);
	*video++ = attr;
	*video++ = HEX(v >> 8);
	*video++ = attr;
	*video++ = HEX(v >> 4);
	*video++ = attr;
	*video++ = HEX(v >> 0);
	*video++ = attr;
}

void print_ptr(int x, int y, void* ptr, char attr) {
	char* video = (char*)0xb8000 + (y * 160) + x * 2;
	
	#define HEX(v)  (((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
	uint8_t* p = ptr;
	for(int i = 0; i < 8; i++) {
		*video++ = HEX(*p >> 4);
		*video++ = attr;
		*video++ = HEX(*p);
		*video++ = attr;
		p++;
	}
}
