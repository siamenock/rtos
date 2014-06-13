#include "page.h"
#include "pnkc.h"
#include "bfs.h"
#include "mode_switch.h"

#define NORMAL	0x07
#define PASS	0x02
#define FAIL	(0x04 | 0x08)

static uint8_t get_core_id();
static void print(int x, int y, const char* str, char attr);
static void print_32(int x, int y, uint32_t v, char attr);
static void print_64(int x, int y, uint64_t v, char attr);
static void print_ptr(int x, int y, void* ptr, char attr);
static int check_memory_size();
static void get_cpu_vendor_string(char* str);
static int check_x86_64_support();
static PNKC* find_kernel();
static int copy_kernel(PNKC* pnkc, int x, int y);
static int copy_kernel_half(PNKC* pnkc, uint8_t core_id);
static void dump(int x, int y, void* addr);

void main(void) {
	uint8_t core_id = get_core_id();
	
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
		
		print(0, 4, "Checking x86_64 supported", NORMAL);
		if(check_x86_64_support()) {
			print(50, 4, "Pass", PASS);
		} else {
			print(50, 4, "Fail", FAIL);
			while(1) asm("hlt");
		}
		
		print(0, 5, "Copying kernel", NORMAL);
		PNKC* pnkc = find_kernel();
		if(pnkc == 0) {
			print(50, 5, "Fail", FAIL);
			while(1) asm("hlt");
		}
		
		if(copy_kernel(pnkc, 50, 5)) {
			print(50, 5, "Pass", PASS);
		} else {
			print(50, 5, "Fail", FAIL);
			while(1) asm("hlt");
		}
		
		print(0, 6, "Initialize memory page tables", NORMAL);
		if(init_page_tables(core_id)) {
			print(50, 6, "Pass", PASS);
		} else {
			print(50, 6, "Fail", FAIL);
			while(1) asm("hlt");
		}
		
		print(0, 7, "Switching to 64 bit mode...", NORMAL);
		asm("nop" : : "d"((uint32_t)core_id));
		mode_switch();
	} else { // AP routine
		PNKC* pnkc = find_kernel();
		copy_kernel_half(pnkc, core_id);
		init_page_tables(core_id);
		asm("nop" : : "d"((uint32_t)core_id));
		mode_switch();
	}
}

uint8_t get_core_id() {
	uint32_t a, b, c, d;
	asm("cpuid" 
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d) 
		: "a"(1));
	
	return (b >> 24) & 0xff;
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

PNKC* find_kernel() {
	void* find_root() {
		void* addr = (void*)((uint32_t)main & ~0x1ff);
		
		for(int i = 0; i < 20; i++) {
			if(*(uint32_t*)addr == BFS_MAGIC) {
				return addr;
			}
			
			addr += 512;
		}
		
		return 0;
	}
	
	void* mmap_kernel(void* bfs) {
		File root, file;
		if(bfs_opendir(bfs, "/", &root) != 0)
			return 0;
		
		while(bfs_readdir(bfs, &root, &file) == 0) {
			if(file.type == FS_TYPE_FILE && 
					file.name[0] == 'k' &&
					file.name[1] == 'e' &&
					file.name[2] == 'r' &&
					file.name[3] == 'n' &&
					file.name[4] == 'e' &&
					file.name[5] == 'l' &&
					file.name[6] == '.' &&
					file.name[7] == 'b' &&
					file.name[8] == 'i' &&
					file.name[9] == 'n') {
				
				return bfs_mmap(bfs, &file, 0);
			}
		}
		
		return 0;
	}
	
	void* bfs = find_root();
	if(!bfs)
		return 0;
	
	return mmap_kernel(bfs);
}

int copy_kernel(PNKC* pnkc, int x, int y) {
	int animate = 0;
	char animation[4] = { '-', '/', '|', '\\' };
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
	
	clean((uint32_t*)0x200000, 0x400000);
	
	// Calc kernel position
	void* body = (void*)pnkc + sizeof(PNKC);
	
	// Copy .text
	void* src = body;
	memcpy((uint32_t*)(0x200000 + pnkc->text_offset), (uint32_t*)src, pnkc->text_size);
	src += pnkc->text_size;
	
	// Copy .rodata
	memcpy((uint32_t*)(0x200000 + pnkc->rodata_offset), (uint32_t*)src, pnkc->rodata_size);
	src += pnkc->rodata_size;
	
	// Copy .data
	memcpy((uint32_t*)(0x400000 + pnkc->data_offset), (uint32_t*)src, pnkc->data_size);
	
	// .bss is already inited
	
	return 1;
}

int copy_kernel_half(PNKC* pnkc, uint8_t core_id) {
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
		}
	}
	
	clean((uint32_t*)(0x400000 + core_id * 0x200000), 0x200000);
	
	// Calc kernel position
	void* body = (void*)pnkc + sizeof(PNKC);
	
	// Copy .data
	void* src = body + pnkc->text_size + pnkc->rodata_size;
	memcpy((uint32_t*)(0x400000 + core_id * 0x200000 + pnkc->data_offset), (uint32_t*)src, pnkc->data_size);
	
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
