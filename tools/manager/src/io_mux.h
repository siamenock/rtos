#ifndef __IOMUX_H__
#define __IOMUX_H__

bool io_mux_poll();
bool io_mux_init();
bool io_mux_add(int fd, void* context, int (*read_handler)(int, void *),
                int (*write_event)(int, void *),
                int (*error_handler)(int, void *));
bool io_mux_remove(void* context);

#endif /*__IOMUX_H__*/