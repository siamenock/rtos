#include <string.h>
#include <malloc.h>
#include "../../loader/src/page.h"
#include "file.h"
#include "vfio.h"
#include "vm.h"

typedef struct {
	VFIO*		fio;
	FIORequest*	vaddr;
	FIORequest*	paddr;
} VFIOContext;

static void vfio_callback(void* buffer, int size, void* context) {
	VFIOContext* ctxt = context;
	VFIO* fio = ctxt->fio;
	FIORequest* vaddr = ctxt->vaddr;
	FIORequest* paddr = ctxt->paddr;

	// Check if user changed the fifo tail on purpose	
	if(fio->output_addr->tail != fio->output_buffer->tail)
		fio->output_addr->tail = fio->output_buffer->tail;

	// Actual size file system has done for file I/O
	paddr->op.file_io.size = size;

	// Sync head
	fio->output_buffer->head = fio->output_addr->head;

	// Push request to output fifo
	fifo_push(fio->output_buffer, vaddr);

	// Sync tail
	fio->output_addr->tail = fio->output_buffer->tail;

	free(context);
}	

static void vfio_sync_callback(int errno, void* context) {
	printf("sync done : %d\n", errno);
}

static void vfio_do_request(VFIO* fio, FIORequest* vaddr, FIORequest* paddr, void* pbuffer, Dirent* pdir) {
	switch(paddr->type) {
		case FILE_T_OPEN:
			paddr->fd = open(paddr->op.open.name, paddr->op.open.flags);
			break;
		case FILE_T_CLOSE:
			paddr->fd = close(paddr->fd);
			break;
		case FILE_T_READ:;
			VFIOContext* read_ctxt = malloc(sizeof(VFIOContext));
			read_ctxt->fio = fio;
			read_ctxt->vaddr = vaddr;
			read_ctxt->paddr = paddr;

			read_async(paddr->fd, pbuffer, paddr->op.file_io.size, vfio_callback, read_ctxt);
			goto exit;
		case FILE_T_WRITE:;
			VFIOContext* write_ctxt = malloc(sizeof(VFIOContext));
			write_ctxt->fio = fio;
			write_ctxt->vaddr = vaddr;
			write_ctxt->paddr = paddr;

			write_async(paddr->fd, pbuffer, paddr->op.file_io.size, vfio_callback, write_ctxt, vfio_sync_callback, NULL);

			goto exit;
		case FILE_T_OPENDIR:
			paddr->fd = opendir(paddr->op.open.name);
			break;
		case FILE_T_CLOSEDIR:
			paddr->fd = closedir(paddr->fd);
			break;
		case FILE_T_READDIR:;
			Dirent* dir = (Dirent*)malloc(sizeof(Dirent));
			if(readdir(paddr->fd, dir) < 0);
			memcpy(pdir, dir, sizeof(Dirent));
			free(dir);
			break;
	}

	// Check if user changed the fifo tail on purpose	
	if(fio->output_addr->tail != fio->output_buffer->tail)
		fio->output_addr->tail = fio->output_buffer->tail;

	// Sync head
	fio->output_buffer->head = fio->output_addr->head;
	
	// Push request to output fifo
	fifo_push(fio->output_buffer, vaddr);

	// Sync tail
	fio->output_addr->tail = fio->output_buffer->tail;

exit:
	return;
}

void vfio_poll(VM* _vm) {
	VM* vm = _vm;
	VFIO* fio = vm->fio;

	// Check if user changed the fifo head on purpose	
	if(fio->input_addr->head != fio->input_buffer->head)
		fio->input_addr->head = fio->input_buffer->head;
	
	// Sync tail
	fio->input_buffer->tail = fio->input_addr->tail;

	// Pop request from input fifo
	FIORequest* vaddr= fifo_pop(fio->input_buffer);

	// Sync head
	fio->input_addr->head = fio->input_buffer->head;

	// If request is nothing, drop it
	if(!vaddr)
		return;

	// Translate virtual address of request, buffer, dirent into physical
	FIORequest* paddr = (FIORequest*)TRANSLATE_TO_PHYSICAL_BASE((uint64_t)vaddr, vm->cores[0]);
	
	Dirent* pdir = NULL;
	if(paddr->type == FILE_T_READDIR) {
		pdir = (Dirent*)TRANSLATE_TO_PHYSICAL_BASE((uint64_t)paddr->op.read_dir.dir, vm->cores[0]);
	}

	void* pbuffer = NULL;
	if(paddr->type == FILE_T_READ || paddr->type == FILE_T_WRITE) {
		pbuffer = (void*)TRANSLATE_TO_PHYSICAL_BASE((uint64_t)paddr->op.file_io.buffer, vm->cores[0]);

		// Check if it's user area memory
		for(int i = 0; i < vm->memory.count; i++) {
			if(pbuffer >= vm->memory.blocks[i] && pbuffer < vm->memory.blocks[i] + 0x200000) {
				goto out;
			}
		}

		pbuffer = NULL;
	}

out:
	vfio_do_request(fio, vaddr, paddr, pbuffer, pdir);
}
