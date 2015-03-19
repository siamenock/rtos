#include <linux/io.h>

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
