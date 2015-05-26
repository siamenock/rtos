#include <linux/io.h>
#include <port.h>

volatile void* ioremap_nocache(resource_size_t offset, unsigned long size) {
	return (void*)offset;
}

volatile void* ioremap_cache(resource_size_t offset, unsigned long size) {
	return (void*)offset;
}

volatile void* ioremap_prot(resource_size_t offset, unsigned long size, unsigned long prot_val) {
	return (void*)offset;
}

void iounmap(volatile void* addr) {}

u8 readb(void* addr) {
	return *(u8*)addr;
}

u16 readw(void* addr) {
	return *(u16*)addr;
}

u32 readl(void* addr) {
	return *(u32*)addr;
}

void writeb(u8 v, void* addr) {
	*(volatile u8*)addr = v;
}

void writew(u16 v, void* addr) {
	*(volatile u16*)addr = v;
}

void writel(u32 v, void* addr) {
	*(volatile u32*)addr = v;
}

unsigned long virt_to_phys(volatile void* address) {
	return (unsigned long)address;
}

void *phys_to_virt(phys_addr_t address) {
	return (void*)((unsigned long)address);
}

u8 ioread8(const volatile void *addr) { 
	return port_in8((uint8_t)(uint64_t)addr);
//	return *(const volatile u8*)addr;
}

u16 ioread16(const volatile void *addr) {
	return port_in16((uint16_t)(uint64_t)addr);
	//return *(const volatile u16*)addr;
}

u32 ioread32(const volatile void *addr) {
	return port_in32((uint32_t)(uint64_t)addr);
	//return *(const volatile u32*)addr;
}

void iowrite8(u8 value, volatile void *addr) {
	port_out8((uint8_t)(uint64_t)addr, value);
	//*(volatile u8*)addr = value;
}

void iowrite16(u16 value, volatile void *addr) {
	port_out16((uint16_t)(uint64_t)addr, value);
	//*(volatile u16*)addr = value;
}

void iowrite32(u32 value, volatile void *addr) {
	port_out32((uint32_t)(uint64_t)addr, value);
	//*(volatile u32*)addr = value;
}
