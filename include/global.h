#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#define SELECTOR_K_CODE  0x08  /*  00001_0_00 */
#define SELECTOR_K_DATA  0x10  /*  00010_0_00 */
#define SELECTOR_K_STACK 0x10  /*  00010_0_00 */
#define SELECTOR_K_GS    0x18  /*  00011_0_00 */
#define SELECTOR_K_TSS   0x20  /*  00100_0_00 */
#define SELECTOR_U_CODE  0x2b  /*  00101_0_11 */
#define SELECTOR_U_DATA  0x33  /*  00110_0_11 */

#define IDT_DESC_ATTR_DPL0 0x8e
#define IDT_DESC_ATTR_DPL3 0xee

#define GDT_BASE 0xc0000900

#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_ON (1 << 9)
#define EFLAGS_IF_OFF 0
#define EFLAGS_IOPL_3 (3 << 12)
#define EFLAGS_IOPL_0 0

#define DIV_ROUND_UP(x, step) (((x) + (step) - 1) / (step))

#define CEIL(x, step) (DIV_ROUND_UP(x, step) * (step))

#define UNUSED __attribute__((unused))
#define NULL (void*)0
#define bool char
#define true 1
#define false 0

#include "debug.h"

#define MEMORY_OK(ptr) ({\
if(!(ptr))\
	PANIC("no more memory\n");\
})

#endif
