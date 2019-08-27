#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H

#include "stdint.h"
#include "global.h"

bool is_pipe(uint32_t l_fd);
int32_t sys_pipe(int32_t pipefd[2]);
uint32_t pipe_read(int32_t fd, void *_buf, uint32_t cnt);
uint32_t pipe_write(int32_t fd, const void *_buf, uint32_t cnt);
void sys_fd_redirect(uint32_t old_fd, uint32_t new_fd);
void pipe_close(int32_t fd);


#endif
