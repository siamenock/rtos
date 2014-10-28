#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <util/event.h>
#include "asm.h"
#include "gdt.h"
#include "idt.h"
#include "pci.h"
#include "stdio.h"
#include "shell.h"
#include "mp.h"
#include "acpi.h"
#include "apic.h"
#include "ioapic.h"
#include "task.h"
#include "cpu.h"
#include "shared.h"
#include "malloc.h"
#include "gmalloc.h"
#include "ni.h"
#include "vm.h"
#include "manager.h"
#include "icc.h"
#include "loader.h"
#include "module.h"
#include "symbols.h"
#include "device.h"
#include "driver/dummy.h"
#include "driver/nic.h"
#include "rootfs.h"

#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <util/cmd.h>

#define VGA_BUFFER_PAGES	12

//static uint64_t idle_time;

static Device* nics[8];
static int nics_count;

static bool idle0_event(void* data) {
	/*
	static uint64_t tick;
	uint64_t time = cpu_tsc();
	
	if(time > tick) {
		tick = time + cpu_frequency;
		ni_statistics(time);
		printf("\033[0;68HLoad: %3d%%", (cpu_frequency - idle_time) * 100 / cpu_frequency);
		idle_time = 0;
		return;
	}
	*/
	
	// Poll NICs
	extern int ni_port;
	int poll_count = 0;
	for(int i = 0; i < nics_count; i++) {
		Device* dev = nics[i];
		NIC* nic = dev->driver;
		
		ni_port = i;
		poll_count += nic->poll(dev->id);
	}
	
	// idle
	/*
	for(int i = 0; i < 100; i++)
		asm volatile("nop");
	*/
	
	//idle_time += cpu_tsc() - time;
	return true;
}

static bool idle_event(void* data) {
	for(int i = 0; i < 100; i++)
		asm volatile("nop");
	
	return true;
}

static void context_switch() {
	// Set exception handlers
	APIC_Handler old_exception_handlers[32];
	
	void exception_handler(uint64_t vector, uint64_t err) {
		printf("* User VM exception handler");
		if(err != 0) {	// Err zero means, user vm termination
			apic_dump(vector, err);
			while(1);
			errno = err;
		}
		
		apic_eoi();
		
		task_destroy(1);
	}
	
	for(int i = 0; i < 32; i++) {
		if(i != 7) {
			old_exception_handlers[i] = apic_register(i, exception_handler);
		}
	}
	
	// Context switching
	// TODO: Move exception handlers to task resources
	task_switch(1);
	
	// Restore exception handlers
	for(int i = 0; i < 32; i++) {
		if(i != 7) {
			apic_register(i, old_exception_handlers[i]);
		}
	}
	
	// Send callback message
	bool is_paused = errno == 0 && task_is_active(1);
	if(is_paused) {
		// ICC_TYPE_PAUSE is not a ICC message but a interrupt in fact, So forcely commit the message
		icc_msg->status = ICC_STATUS_DONE;
	}
	
	ICC_Message* msg3 = icc_sending(is_paused ? ICC_TYPE_PAUSED : ICC_TYPE_STOPPED, 0);
	msg3->result = errno;
	icc_send(msg3);
	errno = 0;
	
	printf("VM %s...\n", is_paused ? "paused" : "stopped");
}

static void icc_start(ICC_Message* msg) {
	printf("Loading VM... \n");
	VM* vm = msg->data.start.vm;
	
	// TODO: Change blocks[0] to blocks
	uint32_t id = loader_load(vm);
	if(errno != 0) {
		ICC_Message* msg2 = icc_sending(ICC_TYPE_STARTED, msg->core_id);
		msg->status = ICC_STATUS_DONE;
		
		msg2->result = errno;	// errno from loader_load
		icc_send(msg2);
		printf("Execution FAILED: %x\n", errno);
	}
	
	*(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_size;
	NetworkInterface** nis = (NetworkInterface**)task_addr(id, SYM_NIS);
	for(uint32_t i = 0; i < vm->nic_size; i++) {
		task_resource(id, RESOURCE_NI, vm->nics[i]);
		nis[i] = vm->nics[i]->ni;
	}
	
	printf("Starting VM...\n");
	ICC_Message* msg2 = icc_sending(ICC_TYPE_STARTED, msg->core_id);
	msg->status = ICC_STATUS_DONE;
	
	msg2->result = 0;
	msg2->data.started.stdin = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDIN));
	msg2->data.started.stdin_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_HEAD));
	msg2->data.started.stdin_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_TAIL));
	msg2->data.started.stdin_size = *(int*)task_addr(id, SYM_STDIN_SIZE);
	msg2->data.started.stdout = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDOUT));
	msg2->data.started.stdout_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_HEAD));
	msg2->data.started.stdout_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_TAIL));
	msg2->data.started.stdout_size = *(int*)task_addr(id, SYM_STDOUT_SIZE);
	msg2->data.started.stderr = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDERR));
	msg2->data.started.stderr_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_HEAD));
	msg2->data.started.stderr_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_TAIL));
	msg2->data.started.stderr_size = *(int*)task_addr(id, SYM_STDERR_SIZE);
	
	icc_send(msg2);
	
	context_switch();
}

static void icc_resume(ICC_Message* msg) {
	printf("Resuming VM...\n");
	ICC_Message* msg2 = icc_sending(ICC_TYPE_RESUMED, msg->core_id);
	msg->status = ICC_STATUS_DONE;
	icc_send(msg2);
	
	context_switch();
}

static void icc_pause(uint64_t vector, uint64_t error_code) {
	apic_eoi();
	
	task_switch(0);
}

static void icc_stop(ICC_Message* msg) {
	msg->status = ICC_STATUS_DONE;
	
	task_destroy(1);
}

static void exec(char* name) {
	uint32_t len;
	char* end = rootfs_file(name, &len);

	if(!end) //init.psh is not exist
		return;

	char* eof = end + len;
	char* head = end;
	char line[CMD_SIZE];

	bool exec_line(void) {
		int line_len = end - head;
		if(line_len < CMD_SIZE) {
			memcpy(line, head, line_len);
			line[line_len] = '\0';
			printf("%s\n", line);
			int ret = cmd_exec(line);
			if(ret != 0 && ret != CMD_STATUS_ASYNC_CALL) {
				printf("Error : Command return %d : Stop parsing %s\n", ret, name);
				return false;
			}
		} else {
			printf("Command line is too long %d > %d\n", line_len, CMD_SIZE);
			return false;
		}
		return true;
	}

	for(; end < eof; end++) {
		if(*end == '\n' || *end == '\0') {
			if(!exec_line())
				return;
			head = end + 1;
		}
	}
	if(head < end)
		exec_line();
}

void main(void) {
	cpu_init();
	mp_init0();
	
	uint8_t core_id = mp_core_id();
	if(core_id == 0) {
		// Bootstrap processor
		stdio_init();
		
		printf("Initializing shared area...\n");
		shared_init();
		
		printf("Initializing malloc...\n");
		malloc_init();
		
		printf("Initializing gmalloc...\n");
		gmalloc_init();
		stdio_init2(gmalloc(4000 * VGA_BUFFER_PAGES), 4000 * VGA_BUFFER_PAGES);
		
		printf("Analyzing CPU information...\n");
		cpu_info();
		
		printf("Analyzing multi-processors...\n");
		mp_analyze();
		
		printf("Initialize GDT...\n");
		gdt_init();
		gdt_load();
		
		printf("Loading TSS...\n");
		tss_init();
		tss_load();
		
		printf("Initializing IDT...\n");
		idt_init();
		idt_load();
		
		printf("Initializing ACPI...\n");
		acpi_init();
		
		printf("Initializing PCI...\n");
		pci_init();
		
		printf("Initializing APICs...\n");
		apic_activate();
		
		printf("Initailizing multi-processing...\n");
		mp_init();
		
		printf("Initailizing local APIC...\n");
		apic_init();
		
		printf("Initializing I/O APIC...\n");
		ioapic_init();
		apic_enable();
		
		printf("Initializing Multi-tasking...\n");
		task_init();
		
		printf("Initializing events...\n");
		event_init();
		
		printf("Initializing inter-core communications...\n");
		icc_init();
		
		printf("Initializing kernel symbols...\n");
		symbols_init();
		
		printf("Initializing modules...\n");
		module_init();
		
		printf("Initializing network interface...\n");
		ni_init0();
		
		mp_sync();
		
		printf("Cleaning up memory...\n");
		gmalloc_extend();
		
		printf("Initializing device drivers...");
		int count = device_module_init();
		
		nics_count = device_count(DEVICE_TYPE_NIC);
		printf("Finding NICs: %d\n", nics_count);
		for(int i = 0; i < count; i++) {
			nics[i] = device_get(DEVICE_TYPE_NIC, i);
			NICStatus status;
			((NIC*)nics[i]->driver)->get_status(nics[i]->id, &status);
			printf("\tNIC[%d]: %02x:%02x:%02x:%02x:%02x:%02x %c\n", i,
				(status.mac >> 40) & 0xff, 
				(status.mac >> 32) & 0xff, 
				(status.mac >> 24) & 0xff, 
				(status.mac >> 16) & 0xff, 
				(status.mac >> 8) & 0xff, 
				(status.mac >> 0) & 0xff,
				ni_mac == 0 ? '*' : ' ');
			
			if(ni_mac == 0)
				ni_mac = status.mac;
		}
		
		printf("Initializing VM manager...\n");
		vm_init();
		
		printf("Initializing RPC manager...\n");
		manager_init();
		
		printf("Initializing shell...\n");
		shell_init();
		
		event_busy_add(idle0_event, NULL);
		
		dummy_init();	// There is no meaning
	} else {
		// Application Processor
		mp_wakeup();
		malloc_init();
		mp_analyze();
		gdt_load();
		tss_load();
		idt_load();
		apic_init();
		apic_enable();
		task_init();
		mp_sync();
		event_init();
		icc_init();
		stdio_init();
		icc_register(ICC_TYPE_START, icc_start);
		icc_register(ICC_TYPE_RESUME, icc_resume);
		icc_register(ICC_TYPE_STOP, icc_stop);
		apic_register(49, icc_pause);
		
		event_idle_add(idle_event, NULL);
		/*
		bool tick(void* context) {
			static int counter;
			printf("[%d] Tick %d\n", mp_core_id(), counter++);
			return true;
		}
		event_timer_add(tick, NULL, 10000000, 10000000);
		*/
	}
	
	mp_sync();
	
	if(core_id == 0)
		exec("init.psh");
	
	while(1)
		event_loop();
}
