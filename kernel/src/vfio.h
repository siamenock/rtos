#ifndef __VFIO_H__
#define __VFIO_H__

#include <util/fifo.h>
#include <fio.h>

typedef struct {
	// Physical address pointer
	FIO*			user_fio;

	// Physical address pointer
	FIFO*			input_addr;		///< Input buffer physical address
	FIFO*			output_addr;		///< Output buffer physical address

	// Clone
	FIFO*			input_buffer;		///< Clone of input buffer
	FIFO*			output_buffer;		///< Clone of output buffer

	// Request ID
	uint32_t		request_id;		///< Request id
} VFIO;

typedef struct _VM VM;

void vfio_poll(VM* _vm);

#endif /* __VFIO_H__ */
