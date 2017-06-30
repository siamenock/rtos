#include <stdio.h>
#include "loader.h"
#include "page.h"
#include "errno.h"
#include "task.h"
#include "apic.h"
#include "icc.h"
#include "icc_ap.h"

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
		// ICC_TYPE_PAUSE is not a ICC message but a interrupt in fact, 
		// so forcely commit the message
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
	VM* vm = msg->data.start.vm;
	printf("Loading VM... \n");

	// TODO: Change blocks[0] to blocks
	uint32_t id = loader_load(vm);

	if(errno != 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);

		msg2->result = errno;	// errno from loader_load
		icc_send(msg2, msg->apic_id);
		icc_free(msg);
		printf("Execution FAILED: %x\n", errno);
		return;
	}

	*(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_count;
	NIC** nics = (NIC**)task_addr(id, SYM_NIS);
	for(int i = 0; i < vm->nic_count; i++) {
		task_resource(id, RESOURCE_NI, vm->nics[i]);
		nics[i] = vm->nics[i]->nic;
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

	msg2->data.started.global_heap_idx = TRANSLATE_TO_PHYSICAL((uint64_t)*(uint64_t*)task_addr(id, SYM_GMALLOC_POOL)) >> 21;

	icc_send(msg2, msg->apic_id);

	icc_free(msg);

	context_switch();
}

static void icc_resume(ICC_Message* msg) {
	if(msg->result < 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);
		msg2->result = msg->result;
		icc_send(msg2, msg->apic_id);

		icc_free(msg);
		return;
	}

	printf("Resuming VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);

	icc_send(msg2, msg->apic_id);
	icc_free(msg);
	context_switch();
}

static void icc_pause(uint64_t vector, uint64_t error_code) {
	apic_eoi();
	task_switch(0);
}

static void icc_stop(ICC_Message* msg) {
	if(msg->result < 0) { // Not yet core is started.
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOPPED);
		msg2->result = msg->result;
		icc_send(msg2, msg->apic_id);

		icc_free(msg);
		return;
	}

	icc_free(msg);
	task_destroy(1);
}

int icc_ap_init() {
		icc_register(ICC_TYPE_START, icc_start);
		icc_register(ICC_TYPE_RESUME, icc_resume);
		icc_register(ICC_TYPE_STOP, icc_stop);
		apic_register(49, icc_pause);

		return 0;
}