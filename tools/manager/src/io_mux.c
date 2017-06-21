#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include <util/event.h>
#include <util/map.h>
#include "io_mux.h"

typedef struct _IOMultiplexer {
    void* context; // Key
    int fd;
    int (*read_handler)(int fd, void* context);
    int (*write_event)(int fd, void* context);
    int (*error_handler)(int fd, void* context);
} IOMultiplexer;

static fd_set fds;
static int max_fd;
static struct timeval tv; //Busy Poll
static Map* io_mux_table;

bool io_mux_poll(void* context) {
    fd_set read_fds, write_fds;
    read_fds = write_fds = fds;

    int retval = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);

    if(retval == -1) {
        perror("IO Mux Error\n");
    } else if(retval) {
        MapIterator iter;
        map_iterator_init(&iter, io_mux_table);
        while(map_iterator_has_next(&iter)) {
            MapEntry* entry = map_iterator_next(&iter);
            IOMultiplexer* io_mux = entry->data;

            //Read Handle
            if(FD_ISSET(io_mux->fd, &read_fds)) {
                if(io_mux->read_handler(io_mux->fd, io_mux->context) < 0) {
                    perror("IO Mux Read Error\n");
                }
            }

            //Write Handle
            if(FD_ISSET(io_mux->fd, &write_fds)) {
                if(io_mux->write_event(io_mux->fd, io_mux->context) < 0) {
                    perror("IO Mux Read Error\n");
                }
            }

            //TODO: Error Handle
        }
    }

    return true;
}

bool io_mux_init() {
    FD_ZERO(&fds);
    io_mux_table = map_create(16, NULL, NULL, NULL);
    if(!io_mux_table) return false;

    event_busy_add(io_mux_poll, NULL);
    return true;
}

bool io_mux_add(int fd, void* context, int (*read_handler)(int, void *),
                int (*write_event)(int, void*),
                int (*error_handler)(int, void*)) {
    IOMultiplexer* io_mux = malloc(sizeof(io_mux));
    if(!io_mux) return false;

    io_mux->fd = fd;
    io_mux->context = context;
    io_mux->read_handler = read_handler;
    io_mux->write_event = write_event;
    io_mux->error_handler = error_handler;

    if(!map_put(io_mux_table, io_mux->context, io_mux)) return false;

    FD_SET(io_mux->fd, &fds);
    if(max_fd < io_mux->fd) max_fd = io_mux->fd;

    return true;
}

bool io_mux_remove(void* context) {
    IOMultiplexer* io_mux = map_remove(io_mux_table, context);
    FD_CLR(io_mux->fd, &fds);
    free(io_mux);

    return true;
}