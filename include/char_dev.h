#ifndef __DEVICE_CHAR_DEV_H
#define __DEVICE_CHAR_DEV_H

#include <stdint.h>

typedef int32_t (*chdev_readf) (char *data, uint32_t size);
typedef int32_t (*chdev_writef)(const char *data, uint32_t size);

extern chdev_readf chdev_rdtlb[7];
extern chdev_writef chdev_wrtlb[7];

void keyboard_init();
int32_t kb_read(char *buf, uint32_t cnt);
#endif
