#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "stdint.h"

void terminal_init(void);
void terminal_writestr(const char* data);
void terminal_clear();

#endif
