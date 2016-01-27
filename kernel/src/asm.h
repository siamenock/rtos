#ifndef __ASM_H__
#define __ASM_H__

#include <stdint.h>
#include <stdbool.h>

void lgdt(void* gdt_address);
void ltr(uint16_t ts_segment);
void lidt(void* idt_address);
void sti();
void cli();
uint64_t pushfq();
uint64_t rdtsc();
void hlt();
//bool cmpxchg(volatile uint8_t* s1, uint8_t s2, uint8_t d);
void clflush(void* addr);
void wbinvd();
uint64_t read_cr0();
uint64_t read_cr2();
void write_cr2(uint64_t cr2);
uint64_t read_cr3();
void write_cr3(uint64_t cr3);
void refresh_cr3();
void* read_rbp();
void* read_rsp();
void read_xmms0(void* v);
void mwait(uint32_t ecx, uint32_t eax);
void monitor(void* addr);
uint64_t read_msr(uint32_t addr);
uint64_t write_msr(uint32_t addr, uint64_t val);

#endif /* __ASM_H__ */
