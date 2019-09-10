#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define printk printf
#define ASSERT(x) (0)
#define PANIC(...) (0)
#define DIV_ROUND_UP(x, step) (((x) + (step) - 1) / (step))
#define kmalloc malloc
#define kfree free
#define MEMORY_OK(x) (0)

#endif
