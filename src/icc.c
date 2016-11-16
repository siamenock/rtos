#include <stdio.h>
#include <string.h>
#include <lock.h>
#include <util/fifo.h>
#include <util/event.h>
#include <_malloc.h>
#include "apic.h"
#include "shared.h"
#include "task.h"
#include "mmap.h"
/*
 *#include <util/event.h>
 *#include <lock.h>
 *#include <_malloc.h>
 *#include <timer.h>
 *#include "asm.h"
 *#include "mp.h"
 *#include "gmalloc.h"
 */

#include "icc.h"

static uint32_t icc_id;

#define ICC_EVENTS_COUNT	64

typedef void (*ICC_Handler)(ICC_Message*);
static ICC_Handler icc_events[ICC_EVENTS_COUNT];

static void context_switch() {
	/*// Set exception handlers*/
	/*APIC_Handler old_exception_handlers[32];*/
	
	/*void exception_handler(uint64_t vector, uint64_t err) {*/
		/*if(apic_user_rip() == 0 && apic_user_rsp() == task_get_stack(1)) {*/
			/*// Do nothing*/
		/*} else {*/
			/*printf("* User VM exception handler");*/
			/*apic_dump(vector, err);*/
			/*errno = err;*/
		/*}*/
		
		/*apic_eoi();*/
		
		/*task_destroy(1);*/
	/*}*/
	
	/*for(int i = 0; i < 32; i++) {*/
		/*if(i != 7) {*/
			/*old_exception_handlers[i] = apic_register(i, exception_handler);*/
		/*}*/
	/*}*/
	
	/*// Context switching*/
	/*// TODO: Move exception handlers to task resources*/
	/*task_switch(1);*/
	
	/*// Restore exception handlers*/
	/*for(int i = 0; i < 32; i++) {*/
		/*if(i != 7) {*/
			/*apic_register(i, old_exception_handlers[i]);*/
		/*}*/
	/*}*/
	
	/*// Send callback message*/
	/*bool is_paused = errno == 0 && task_is_active(1);*/
	/*if(is_paused) {*/
		/*// ICC_TYPE_PAUSE is not a ICC message but a interrupt in fact, So forcely commit the message*/
	/*}*/
	
	/*ICC_Message* msg3 = icc_alloc(is_paused ? ICC_TYPE_PAUSED : ICC_TYPE_STOPPED);*/
	/*msg3->result = errno;*/
	/*if(!is_paused) {*/
		/*msg3->data.stopped.return_code = apic_user_return_code();*/
	/*}*/
	/*errno = 0;*/
	/*icc_send(msg3, 0);*/
	
	/*printf("VM %s...\n", is_paused ? "paused" : "stopped");*/
}

static void icc_start(ICC_Message* msg) {
	printf("Loading VM... \n");
/*
 *        VM* vm = msg->data.start.vm;
 *
 *        // TODO: Change blocks[0] to blocks
 *        uint32_t id = loader_load(vm);
 *
 *        if(errno != 0) {
 *                ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);
 *
 *                msg2->result = errno;	// errno from loader_load
 *                icc_send(msg2, msg->apic_id);
 *                icc_free(msg);
 *                printf("Execution FAILED: %x\n", errno);
 *                return;
 *        }
 *
 *        *(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_count;
 *        NIC** nics = (NIC**)task_addr(id, SYM_NIS);
 *        for(int i = 0; i < vm->nic_count; i++) {
 *                task_resource(id, RESOURCE_NI, vm->nics[i]);
 *                nics[i] = vm->nics[i]->nic;
 *        }
 *                
 *        printf("Starting VM...\n");
 *        ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);
 *        
 *        msg2->data.started.stdin = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDIN));
 *        msg2->data.started.stdin_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_HEAD));
 *        msg2->data.started.stdin_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_TAIL));
 *        msg2->data.started.stdin_size = *(int*)task_addr(id, SYM_STDIN_SIZE);
 *        msg2->data.started.stdout = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDOUT));
 *        msg2->data.started.stdout_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_HEAD));
 *        msg2->data.started.stdout_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_TAIL));
 *        msg2->data.started.stdout_size = *(int*)task_addr(id, SYM_STDOUT_SIZE);
 *        msg2->data.started.stderr = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDERR));
 *        msg2->data.started.stderr_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_HEAD));
 *        msg2->data.started.stderr_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_TAIL));
 *        msg2->data.started.stderr_size = *(int*)task_addr(id, SYM_STDERR_SIZE);
 *        
 *        icc_send(msg2, msg->apic_id);
 *
 *        icc_free(msg);
 *        
 *        context_switch();
 */
}

/*
 *static void icc_resume(ICC_Message* msg) {
 *        if(msg->result < 0) {
 *                ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);
 *                msg2->result = msg->result;
 *                icc_send(msg2, msg->apic_id);
 *
 *                icc_free(msg);
 *                return;
 *        }
 *
 *        printf("Resuming VM...\n");
 *        ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);
 *
 *        icc_send(msg2, msg->apic_id);
 *        icc_free(msg);
 *        context_switch();
 *}
 *
 *static void icc_pause(uint64_t vector, uint64_t error_code) {
 *        printf("Interrupt occured!\n");
 *        apic_eoi();
 *        //task_switch(0);
 *}
 *
 *static void icc_stop(ICC_Message* msg) {
 *        if(msg->result < 0) { // Not yet core is started.
 *                ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOPPED);
 *                msg2->result = msg->result;
 *                icc_send(msg2, msg->apic_id);
 *
 *                icc_free(msg);
 *                return;
 *        }
 *
 *        icc_free(msg);
 *        task_destroy(1);
 *}
 */

static bool icc_event(void* context) {
	uint8_t apic_id = mp_apic_id();
	FIFO* icc_queue = shared->icc_queues[apic_id].icc_queue;

	if(fifo_empty(icc_queue))
		return true;

	lock_lock(&shared->icc_queues[apic_id].icc_queue_lock);
	ICC_Message* icc_msg = fifo_pop(icc_queue);
	lock_unlock(&shared->icc_queues[apic_id].icc_queue_lock);

	if(icc_msg == NULL)
		return true;

	printf("ICC Msg popped : %d (TYPE)\n", icc_msg->type);
	if(icc_msg->type >= ICC_EVENTS_COUNT) {
		icc_free(icc_msg);
		return true;
	}

	if(!icc_events[icc_msg->type]) {
		icc_free(icc_msg);
		return true;
	}

	printf("ICC event called\n");
	icc_events[icc_msg->type](icc_msg); //event call

	return true;
}

static void icc(uint64_t vector, uint64_t err) {
	uint8_t apic_id = mp_apic_id();
	FIFO* icc_queue = shared->icc_queues[apic_id].icc_queue;
	ICC_Message* icc_msg = fifo_peek(icc_queue, 0);

	apic_eoi();

	printf("ICC occured\n");
	if(icc_msg == NULL)
		return;

	icc_msg->result = 0;
	if(task_id() != 0) {
		if(icc_msg->type == ICC_TYPE_RESUME) {
			icc_msg->result = -1000;
			icc_event(NULL);
		} else {
			icc_event(NULL);
		}
	} else {
		if(icc_msg->type == ICC_TYPE_STOP) {
			icc_msg->result = -1000;
			icc_event(NULL);
		}
	}
}

void icc_init() {
	uint8_t core_count = mp_core_count();

	extern void* gmalloc_pool;
	uint8_t apic_id = mp_apic_id();
	if(apic_id == 0) {
		int icc_max = core_count * core_count;
		shared->icc_pool = fifo_create(icc_max, gmalloc_pool);
		for(int i = 0; i < icc_max; i++) {
			ICC_Message* icc_message = __malloc(sizeof(ICC_Message), shared->icc_pool->pool);
			fifo_push(shared->icc_pool, icc_message);
		}

		shared->icc_queues = __malloc(MP_MAX_CORE_COUNT * sizeof(Icc), gmalloc_pool);

		lock_init(&shared->icc_lock_alloc);
		uint8_t* core_map = mp_core_map();
		for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
			if(core_map[i] != MP_CORE_INVALID) {
				shared->icc_queues[i].icc_queue = fifo_create(core_count, gmalloc_pool);
				lock_init(&shared->icc_queues[i].icc_queue_lock);
			}
		}
	}
	event_busy_add(icc_event, NULL);
	apic_register(48, icc);
}

ICC_Message* icc_alloc(uint8_t type) {
	lock_lock(&shared->icc_lock_alloc);
	ICC_Message* icc_message = fifo_pop(shared->icc_pool);
	lock_unlock(&shared->icc_lock_alloc);
	
	icc_message->id = icc_id++;
	icc_message->type = type;
	icc_message->apic_id = mp_apic_id();
	icc_message->result = 0;

	return icc_message;
}

void icc_free(ICC_Message* msg) {
	lock_lock(&shared->icc_lock_free);
	fifo_push(shared->icc_pool, msg);
	lock_unlock(&shared->icc_lock_free);
}

uint32_t icc_send(ICC_Message* msg, uint8_t apic_id) {
	printf("ICC send to %d, Type %d\n", apic_id, (msg->type == ICC_TYPE_PAUSE ? 49 : 48));
	uint32_t _icc_id = msg->id;

	lock_lock(&shared->icc_queues[apic_id].icc_queue_lock);
	fifo_push(shared->icc_queues[apic_id].icc_queue, msg);
	lock_unlock(&shared->icc_queues[apic_id].icc_queue_lock);

	apic_write64(APIC_REG_ICR, ((uint64_t)apic_id << 56) |
				APIC_DSH_NONE |
				APIC_TM_EDGE |
				APIC_LV_DEASSERT |
				APIC_DM_PHYSICAL |
				APIC_DMODE_FIXED |
				(msg->type == ICC_TYPE_PAUSE ? 49 : 48));

//	uint64_t time = timer_frequency() + cpu_ms * 100;
//
//	while(timer_frequency() < time);
	// TODO: Remove wait
//	timer_mwait(100);

	return _icc_id;
}

void icc_register(uint8_t type, void(*event)(ICC_Message*)) {
	icc_events[type] = event;
}
