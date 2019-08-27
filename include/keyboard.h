#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H

#include "stdint.h"
void keyboard_init();
uint32_t kb_read(void *_buf, uint32_t cnt);

#endif
