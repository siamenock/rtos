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
#include "vnic.h"
#include "vm.h"
#include "manager.h"
#include "icc.h"
#include "loader.h"
#include "module.h"
#include "symbols.h"
#include "device.h"
#include "driver/usb/usb.h"
#include "driver/pata.h"
#include "driver/fs.h"
#include "driver/bfs.h"
#include "driver/dummy.h"
#include "driver/nic.h"

#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <util/cmd.h>

#define VGA_BUFFER_PAGES	12

//static uint64_t idle_time;
extern Device* nic_devices[];

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
	extern Device* nic_current;
	int poll_count = 0;
	for(int i = 0; i < NIC_MAX_DEVICE_COUNT; i++) {
		Device* dev = nic_devices[i];
		if(dev == NULL)
			break;
		
		nic_current = dev;
		NICDriver* nic = nic_current->driver;
		
		poll_count += nic->poll(nic_current->id);
	}

	// idle
/*	
	for(int i = 0; i < 1000; i++)
		asm volatile("nop");
*/	
	
	//idle_time += cpu_tsc() - time;
	return true;
}

static bool idle_monitor_event(void* data) {
	static uint8_t trigger;

	monitor(&trigger);
	mwait(1, 0x21);

	return true;
}

static bool idle_hlt_event(void* data) {
	hlt();

	return true;
}

static void context_switch() {
	// Set exception handlers
	APIC_Handler old_exception_handlers[32];
	
	void exception_handler(uint64_t vector, uint64_t err) {
		if(apic_user_rip() == 0 && apic_user_rsp() == task_get_stack(1)) {
			// Do nothing
		} else {
			printf("* User VM exception handler");
			apic_dump(vector, err);
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
	}
	
	ICC_Message* msg3 = icc_alloc(is_paused ? ICC_TYPE_PAUSED : ICC_TYPE_STOPPED);
	msg3->result = errno;
	if(!is_paused) {
		msg3->data.stopped.return_code = apic_user_return_code();
	}
	errno = 0;
	icc_send(msg3, 0);
	
	printf("VM %s...\n", is_paused ? "paused" : "stopped");
}

static void icc_start(ICC_Message* msg) {
	printf("Loading VM... \n");
	VM* vm = msg->data.start.vm;
	
	// TODO: Change blocks[0] to blocks
	uint32_t id = loader_load(vm);

	if(errno != 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);

		msg2->result = errno;	// errno from loader_load
		icc_send(msg2, msg->core_id);
		icc_free(msg);
		printf("Execution FAILED: %x\n", errno);
		return;
	}
	
	*(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_count;
	NetworkInterface** nis = (NetworkInterface**)task_addr(id, SYM_NIS);
	for(int i = 0; i < vm->nic_count; i++) {
		task_resource(id, RESOURCE_NI, vm->nics[i]);
		nis[i] = vm->nics[i]->ni;
	}
	
	printf("Starting VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);
	
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
	
	icc_send(msg2, msg->core_id);

	icc_free(msg);
	
	context_switch();
}

static void icc_resume(ICC_Message* msg) {
	if(msg->result < 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);
		msg2->result = msg->result;
		icc_send(msg2, msg->core_id);

		icc_free(msg);
		return;
	}

	printf("Resuming VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);

	icc_send(msg2, msg->core_id);
	icc_free(msg);
	context_switch();
}

static void icc_pause(uint64_t vector, uint64_t error_code) {
	apic_eoi();
	
	task_switch(0);
}

static void icc_stop(ICC_Message* msg) {
	if(msg->result < 0) { //Not yet core is started.
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOPPED);
		msg2->result = msg->result;
		icc_send(msg2, msg->core_id);

		icc_free(msg);
		return;
	}

	icc_free(msg);
	task_destroy(1);
}

static void cmd_callback(char* result, int exit_status) {
	cmd_update_var(result, exit_status);
	printf("%s\n", result);
}

static void exec(char* name) {
	int fd = open(name, "r");
	if(fd < 0) {
		return ;
	}
	char line[CMD_SIZE];
	int eof;
	if((eof = read(fd, line, CMD_SIZE)) <= 0) {
		printf("Read error\n");
		close(fd);
		return ;
	}

	char* ptr = line;
	char* head = ptr;

	bool exec_line(void) {
		int line_len = ptr - head;
		head[line_len] = '\0';
		int ret = cmd_exec(head,cmd_callback);

		if(ret != 0) {
			printf("Exec error\n");
			return false;
		}

		return true;
	}

	for(int i = 0; i < eof; i++, ptr++) {
		if(*ptr == '\n' || *ptr == '\0') {
			if(!exec_line()) {
				close(fd);
				return ;
			}

			head = ptr + 1;
		}
	}

	close(fd);
}

void main(void) {
	cpu_init();
	mp_init0();

	uint8_t core_id = mp_core_id();
	if(core_id == 0) {
		stdio_init();

		// Bootstrap processor
		printf("Initializing shared area...\n");
		shared_init();

		printf("Initializing malloc...\n");
		malloc_init();

		printf("Initializing gmalloc...\n");
		gmalloc_init();
		stdio_init2(gmalloc(4000 * VGA_BUFFER_PAGES), 4000 * VGA_BUFFER_PAGES);

		printf("Initializing USB controller driver...\n");
		usb_initialize();

		printf("Initializing disk drivers...\n");
		disk_init0();
		disk_register(&pata_driver);
		disk_register(&usb_msc_driver);
		disk_init();

		printf("Initializing file system...\n");
		bfs_init();
		fs_init();

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
	
		printf("Initializing network interface...\n");
		vnic_init0();

		mp_sync();
	
		printf("Cleaning up memory...\n");
		gmalloc_extend();

		printf("Initializing modules...\n");
		module_init();

		printf("Initializing device drivers...\n");
		device_module_init(); //

		uint16_t nic_devices_count = device_count(DEVICE_TYPE_NIC);
		printf("Finding NICs: %d\n", nic_devices_count);

		int ni_dev_index = 0;
		extern uint64_t manager_mac;
		for(int i = 0; i < nic_devices_count; i++) {
			nic_devices[i] = device_get(DEVICE_TYPE_NIC, i);
			if(!nic_devices[i])
				continue;

			NICPriv* nic_priv = gmalloc(sizeof(NICPriv));
			if(!nic_priv)
				continue;

			nic_priv->nics = map_create(8, NULL, NULL, NULL);

			nic_devices[i]->priv = nic_priv;

			NICInfo info;
			((NICDriver*)nic_devices[i]->driver)->get_info(nic_devices[i]->id, &info);

			nic_priv->port_count = info.port_count;
			for(int j = 0; j < info.port_count; j++) {
				nic_priv->mac[j] = info.mac[j];

				char name_buf[64];
				sprintf(name_buf, "eth%d", ni_dev_index);
				uint16_t port = j << 12;

				Map* vnics = map_create(16, NULL, NULL, NULL);
				map_put(nic_priv->nics, (void*)(uint64_t)port, vnics);

				printf("\t%s : [%02x:%02x:%02x:%02x:%02x:%02x] [%c]\n", name_buf,
					(info.mac[j] >> 40) & 0xff,
					(info.mac[j] >> 32) & 0xff,
					(info.mac[j] >> 24) & 0xff,
					(info.mac[j] >> 16) & 0xff,
					(info.mac[j] >> 8) & 0xff,
					(info.mac[j] >> 0) & 0xff,
					manager_mac == 0 ? '*' : ' ');

				if(!manager_mac)
					manager_mac = info.mac[j];

				ni_dev_index++;
			}
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
		
		if(CPU_IS_MONITOR_MWAIT & CPU_IS_MWAIT_INTERRUPT)
			event_idle_add(idle_monitor_event, NULL);
		else
			event_idle_add(idle_hlt_event, NULL);
	}

	mp_sync();

	if(core_id == 0)
		exec("/init.psh");

	while(1)
		event_loop();
}
