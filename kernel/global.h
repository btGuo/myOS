#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#define SELECTOR_K_CODE  0x08
#define SELECTOR_K_DATA  0x10
#define SELECTOR_K_STACK 0x10
#define SELECTOR_K_GS    0x18

#define IDT_DESC_ATTR_DPL0 0x8e
#define IDT_DESC_ATTR_DPL3 0xee

#endif
