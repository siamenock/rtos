#include <stdio.h>
#include <util/fifo.h>
#include <tlsf.h>
#include <fio.h>

FIO* fio_create(void* pool) {
	FIO* fio = malloc_ex(sizeof(FIO), pool);
	if(!fio) {
		printf("FIO malloc error\n");
		return NULL;
	}

	fio->input_buffer = fifo_create(FIO_INPUT_BUFFER_SIZE, pool);
	if(!fio->input_buffer) {
		printf("fifo creation error\n");
		free_ex(fio, pool);
		return NULL;
	}

	fio->output_buffer = fifo_create(FIO_OUTPUT_BUFFER_SIZE, pool);
	if(!fio->output_buffer) {
		printf("fifo creation error\n");
		fifo_destroy(fio->input_buffer);
		free_ex(fio, pool);
		return NULL;
	}

	return fio;
}
