#ifndef __IOMUX_H__
#define __IOMUX_H__

typedef struct _IOMultiplexer {
    void* context; // Key
    int fd;
    int (*read_handler)(int fd, void* context);
    int (*write_event)(int fd, void* context);
    int (*error_handler)(int fd, void* context);
} IOMultiplexer;

bool io_mux_poll();
bool io_mux_init();
bool io_mux_add(IOMultiplexer* io_mux, uint64_t key);
IOMultiplexer* io_mux_remove(uint64_t key);

#endif /*__IOMUX_H__*/