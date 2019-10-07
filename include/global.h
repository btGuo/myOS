#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#include <kernelio.h>
#include <stdbool.h>

#define SELECTOR_K_CODE  0x08  /*  00001_0_00 */
#define SELECTOR_K_DATA  0x10  /*  00010_0_00 */
#define SELECTOR_K_STACK 0x10  /*  00010_0_00 */
#define SELECTOR_K_GS    0x18  /*  00011_0_00 */
#define SELECTOR_K_TSS   0x20  /*  00100_0_00 */
#define SELECTOR_U_CODE  0x2b  /*  00101_0_11 */
#define SELECTOR_U_DATA  0x33  /*  00110_0_11 */

#define IDT_DESC_ATTR_DPL0 0x8e
#define IDT_DESC_ATTR_DPL3 0xee

#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_ON (1 << 9)
#define EFLAGS_IF_OFF 0
#define EFLAGS_IOPL_3 (3 << 12)
#define EFLAGS_IOPL_0 0

#define DIV_ROUND_UP(x, step) (((x) + (step) - 1) / (step))

#ifndef NULL
#define NULL (void*)0
#endif

#define st_sizeof(type, member) (sizeof(((type *)0)->member))

#ifndef offsetof
#define offsetof(type, member) ((int)(&((type*)0)->member))
#endif

#define UNUSED __attribute__((unused))

#define MEMORY_OK(ptr) ({\
if(!(ptr))\
	PANIC("no more memory\n");\
})

#endif
