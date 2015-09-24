#include <stdio.h>
#include <stddef.h>
#include <timer.h>
#include "version.h"
#include "mp.h"
#include "port.h"
#include "asm.h"
#include "apic.h"

uint64_t _apic_address;

#define HANDLER_SIZE	64

static APIC_Handler handlers[HANDLER_SIZE];

void apic_activate() {
	// Disable PIT
	#define PIT_FREQUENCY	1193180
	
	#define PIT_COUNTER0	0x40
	#define PIT_COUNTER1	0x41
	#define PIT_COUNTER2	0x42
	#define PIT_COMMAND	0x43
	
	#define PIT_CHANNEL0	0x00
	#define PIT_CHANNEL1	0x40
	#define PIT_CHANNEL2	0x60
	
	#define PIT_LATCH	0x00
	#define PIT_LOW		0x10
	#define PIT_HIGH	0x20
	#define PIT_LOWHIGH	0x30
	
	#define PIT_MODE0	0x00
	#define PIT_MODE1	0x02
	#define PIT_MODE2	0x04
	#define PIT_MODE3	0x06
	#define PIT_MODE4	0x08
	#define PIT_MODE5	0x0a
	#define PIT_MODE6	0x0c
	#define PIT_MODE7	0x0e
	
	#define PIT_BIN		0x00
	#define PIT_BCD		0x01
	
	port_out8(PIT_COMMAND, PIT_CHANNEL0 | PIT_LOWHIGH | PIT_MODE0 | PIT_BIN);
	uint16_t interval = 0;
	port_out8(PIT_COUNTER0, (interval >> 0) & 0xff);
	port_out8(PIT_COUNTER0, (interval >> 8) & 0xff);
	
	// Disable PIC(mask all)
	#define PIC_MASTER_PORT2	0x21
	#define PIC_SLAVE_PORT2		0xa1
	
	port_out8(PIC_MASTER_PORT2, 0xff);
	port_out8(PIC_SLAVE_PORT2, 0xff);

	printf("\tActivate local APICs...\n");

	void _apic_activate();
	_apic_activate();
}

void apic_init() {
	// Dummy interrupt handler
	void dummy_timer_handler(uint64_t vector, uint64_t error_code) {
		// Do nothing
		apic_eoi();
	}
	apic_register(32, dummy_timer_handler);
	
	apic_write32(APIC_REG_SIVR, apic_read32(APIC_REG_SIVR) | 0x100);

	apic_write32(APIC_REG_TPR,  0);
	
	apic_write32(APIC_REG_LVT_TR, apic_read32(APIC_REG_LVT_TR) | APIC_IM_DISABLED);
	apic_write32(APIC_REG_LVT_TSR, apic_read32(APIC_REG_LVT_TSR) | APIC_IM_DISABLED);
	apic_write32(APIC_REG_LVT_PMR, apic_read32(APIC_REG_LVT_PMR) |  APIC_IM_DISABLED);
	apic_write32(APIC_REG_LVT_LINT0R, apic_read32(APIC_REG_LVT_LINT0R) | APIC_IM_DISABLED);
	apic_write32(APIC_REG_LVT_LINT1R, APIC_TM_EDGE | APIC_PP_ACTIVEHIGH | APIC_DMODE_NMI);
	apic_write32(APIC_REG_LVT_ER, apic_read32(APIC_REG_LVT_ER) | APIC_IM_DISABLED);
	
	apic_eoi();
}

inline bool apic_enabled() {
	return !!(pushfq() & 0x200);	// Check IF
}

inline void apic_enable() {
	sti();
}

inline void apic_disable() {
	cli();
}

static bool is_enabled;

void apic_pause() {
	is_enabled = apic_enabled();
	
	if(is_enabled) {
		apic_disable();
	}
}

void apic_resume() {
	if(is_enabled) {
		apic_disable();
		is_enabled = false;
	}
}

APIC_Handler apic_register(uint64_t vector, APIC_Handler handler) {
	APIC_Handler old = handlers[vector];
	handlers[vector] = handler;
	
	return old;
}

inline uint32_t apic_read32(int reg) {
	return *(uint32_t volatile*)(_apic_address + reg);
}

inline void apic_write32(int reg, uint32_t v) {
	*(uint32_t volatile*)(_apic_address + reg) = v;
}

inline uint64_t apic_read64(int reg) {
	uint64_t v = *(uint32_t volatile*)(_apic_address + reg);
	v |= (uint64_t)*(uint32_t volatile*)(_apic_address + reg + 0x10) << 32;
	
	return v;
}

inline void apic_write64(int reg, uint64_t v) {
	*(uint32_t volatile*)(_apic_address + reg + 0x10) = (uint32_t)(v >> 32);
	*(uint32_t volatile*)(_apic_address + reg) = (uint32_t)v;
}

#define HEX(v)	(((v) & 0x0f) > 9 ? ((v) & 0x0f) - 10 + 'a' : ((v) & 0x0f) + '0')
static char animation[] = { '-', '\\', '|', '/' };

typedef struct {
	uint64_t	gs;
	uint64_t	fs;
	uint64_t	es;
	uint64_t	ds;
	uint64_t	r15;
	uint64_t	r14;
	uint64_t	r13;
	uint64_t	r12;
	uint64_t	r11;
	uint64_t	r10;
	uint64_t	r9;
	uint64_t	r8;
	uint64_t	rsi;
	uint64_t	rdi;
	uint64_t	rdx;
	uint64_t	rcx;
	uint64_t	rbx;
	uint64_t	rax;
	uint64_t	rbp;
	
	uint64_t	rip;
	uint64_t	cs;
	uint64_t	rflag;
	uint64_t	rsp;
	uint64_t	ss;
} __attribute__ ((packed)) Frame;

uint64_t apic_user_rip() {
	Frame* frame = (void*)(0xffffffff805b0000 - sizeof(Frame));
	return frame->rip;
}

uint64_t apic_user_rsp() {
	Frame* frame = (void*)(0xffffffff805b0000 - sizeof(Frame));
	return frame->rsp;
}

uint64_t apic_user_return_code() {
	Frame* frame = (void*)(0xffffffff805b0000 - sizeof(Frame));
	return frame->rbx;
}

void apic_dump(uint64_t vector, uint64_t error_code) {
	Frame* frame = (void*)(0xffffffff805b0000 - sizeof(Frame));
	
	printf("\n* Exception: ver=%d.%d.%d-%s core=%d, vector=0x%lx, error=0x%lx\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG, mp_core_id(), vector, error_code);
	printf("AX=%016lx BX=%016lx CX=%016lx DX=%016lx\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
	printf("SI=%016lx DI=%016lx BP=%016lx SP=%016lx\n", frame->rsi, frame->rdi, frame->rbp, frame->rsp);
	printf("8 =%016lx 9 =%016lx 10=%016lx 11=%016lx\n", frame->r8, frame->r9, frame->r10, frame->r11);
	printf("12=%016lx 13=%016lx 14=%016lx 15=%016lx\n", frame->r12, frame->r13, frame->r14, frame->r15);
	printf("IP=%016lx FL=%016lx\n", frame->rip, frame->rflag);
	printf("ES=%08x CS=%08x DS=%08x FS=%08x GS=%08x SS=%08x\n", frame->es, frame->cs, frame->ds, frame->fs, frame->gs, frame->ss);
	
	printf("\n");
	uint64_t* p = (void*)frame->rsp;
	for(int i = -20; i < 0; i += 4) {
		printf("%016lx %016lx %016lx %016lx\n", p[i], p[i + 1], p[i + 2], p[i + 3]);
	}
	printf("----------------------------\n");
	for(int i = 0; i < 40; i += 4) {
		printf("%016lx %016lx %016lx %016lx\n", p[i], p[i + 1], p[i + 2], p[i + 3]);
	}
}

void isr_exception_handler(uint64_t vector, uint64_t error_code) {
	if(vector < HANDLER_SIZE && handlers[vector]) {
		(handlers[vector])(vector, error_code);
	} else {
		apic_disable();
		apic_dump(vector, error_code);
		
		while(1) {
			uint8_t core_id = mp_core_id();
			char* v = (char*)0xb8000 + core_id * 160 + 40 * 2;
			*v++ = HEX(core_id >> 4);
			*v++ = 0x40;
			*v++ = HEX(core_id);
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = 'A';
			*v++ = 0x40;
			*v++ = 'P';
			*v++ = 0x40;
			*v++ = 'I';
			*v++ = 0x40;
			*v++ = 'C';
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = 'E';
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = HEX(vector >> 28);
			*v++ = 0x40;
			*v++ = HEX(vector >> 24);
			*v++ = 0x40;
			*v++ = HEX(vector >> 20);
			*v++ = 0x40;
			*v++ = HEX(vector >> 16);
			*v++ = 0x40;
			*v++ = HEX(vector >> 12);
			*v++ = 0x40;
			*v++ = HEX(vector >> 8);
			*v++ = 0x40;
			*v++ = HEX(vector >> 4);
			*v++ = 0x40;
			*v++ = HEX(vector >> 0);
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = HEX(error_code >> 60);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 56);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 52);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 48);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 44);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 40);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 36);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 32);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 28);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 24);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 20);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 16);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 12);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 8);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 4);
			*v++ = 0x40;
			*v++ = HEX(error_code >> 0);
			*v++ = 0x40;
			
			while(1) asm("nop");
			/*
			for(int i = 0; i < 1000000; i++)
				asm("nop");
			*/
		}
	}
}

void isr_interrupt_handler(uint64_t vector) {
	if(vector < HANDLER_SIZE && handlers[vector]) {
		(handlers[vector])(vector, 0);
	} else {
		apic_dump(vector, 0);
		
		while(1) {
			uint8_t core_id = mp_core_id();
			char* v = (char*)0xb8000 + core_id * 160 + 50 * 2;
			*v++ = HEX(core_id >> 4);
			*v++ = 0x40;
			*v++ = HEX(core_id);
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = 'A';
			*v++ = 0x40;
			*v++ = 'P';
			*v++ = 0x40;
			*v++ = 'I';
			*v++ = 0x40;
			*v++ = 'C';
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = 'I';
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = HEX(vector >> 28);
			*v++ = 0x40;
			*v++ = HEX(vector >> 24);
			*v++ = 0x40;
			*v++ = HEX(vector >> 20);
			*v++ = 0x40;
			*v++ = HEX(vector >> 16);
			*v++ = 0x40;
			*v++ = HEX(vector >> 12);
			*v++ = 0x40;
			*v++ = HEX(vector >> 8);
			*v++ = 0x40;
			*v++ = HEX(vector >> 4);
			*v++ = 0x40;
			*v++ = HEX(vector >> 0);
			*v++ = 0x40;
			*v++ = ' ';
			*v++ = 0x40;
			*v++ = animation[(timer_frequency() >> 16) % 4];
			*v++ = 0x40;
			
			for(int i = 0; i < 1000000; i++)
				asm("nop");
		}
		
		apic_eoi();
	}
}
