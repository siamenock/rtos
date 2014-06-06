#include "linux.h"
#include "../apic.h"

uint64_t jiffies = 0;

void init_timer(struct timer_list *timer) {
	bzero(timer, sizeof(struct timer_list));
	timer->id = (uint64_t)-1;
}

void add_timer(struct timer_list *timer) {
	bool fn(struct timer_list* timer) {
		timer->function(timer->data);
		
		return true;
	}
	
	timer->id = event_tevent((void*)fn, (void*)timer, timer->expires, timer->expires);
}

int mod_timer(struct timer_list *timer, unsigned long expires) {
	if(timer->id == (uint64_t)-1) {
		timer->expires = expires;
		add_timer(timer);
	} else if(timer->expires != expires) {
		event_tevent_cancel(timer->id);
		timer->expires = expires;
		add_timer(timer);
	}
	
	return 0;
}

void del_timer(struct timer_list *timer) {
	event_tevent_cancel(timer->id);
}

void msleep(unsigned int d) {
	cpu_mwait(d);
}

int eth_validate_addr(struct net_device *dev) {
	if (!is_valid_ether_addr(dev->dev_addr))
		return -1;
	
	return 0;
}

//static uint64_t dma_mapping[256][2];

/*
#include <stdio.h>
#include "../../../loader/src/page.h"
void* dma_alloc_coherent(PCI_Device *dev, size_t size, dma_addr_t *dma_handle, int flag) {
	void* bmalloc();
	void* ptr = bmalloc();
	
	uint64_t idx = (uint64_t)ptr >> 21;
	PAGE_L4U[idx].pcd = 1;
	PAGE_L4U[idx].pwt = 1;
	void refresh_cr3();
	refresh_cr3();
	
	*dma_handle = (dma_addr_t)ptr;

	return ptr;
	 
	void* ptr = gmalloc(size + 255);
	uint64_t addr = ((uint64_t)ptr + 255) & ~255ul;
	
	for(int i = 0; i < 256; i++) {
		if(dma_mapping[i][0] == 0) {
			dma_mapping[i][0] = (uint64_t)ptr;
			dma_mapping[i][1] = addr;
		}
	}
	
	*dma_handle = addr;
	return (void*)addr;
}

void dma_free_coherent(PCI_Device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle) {
	uint64_t idx = dma_handle >> 21;
	PAGE_L4U[idx].pcd = 0;
	PAGE_L4U[idx].pwt = 0;
	void refresh_cr3();
	refresh_cr3();
	
	void bfree(void*);
	bfree((void*)dma_handle);
	
	for(int i = 0; i < 256; i++) {
		if(dma_mapping[i][1] == dma_handle) {
			gfree((void*)dma_mapping[i][0]);
			dma_mapping[i][0] = 0;
		}
	}
}
*/

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev) {
	/*
	void apic_handler(uint64_t vector, uint64_t err) {
		handler(vector, NULL);
		apic_eoi();
	}
	
	apic_register(irq, apic_handler);
	*/
	
	return 0;
}

void free_irq(unsigned int irq, void* dev) {
	//apic_register(irq, NULL);
}

#include "firmware.h"

int request_firmware(const struct firmware **fw, const char *name, PCI_Device *device) {
	if(strcmp(name, "rtl_nic/rtl8168e-3.fw") == 0) {
		struct firmware* f = malloc(sizeof(struct firmware));
		f->data = rtl8168e_3;
		f->size = sizeof(rtl8168e_3);
		*(struct firmware**)fw = f;
	} else {
		printf("Unknown firmware: %s\n", name);
		return -1;
	}
	
	return 0;
}

void release_firmware(const struct firmware *fw) {
	free((struct firmware*)fw);
}
