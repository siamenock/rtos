#include "bootparam.h"

int open_mem(int flag);
void close_mem(int fd);
struct boot_params* map_boot_param(int mem_fd);
void unmap_boot_param(struct boot_params* boot_param);
int save_cmd_line(char* buf, int buf_len);
int load_cmd_line(char* buf, int buf_len);
