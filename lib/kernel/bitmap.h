#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include"global.h"
#include"stdint.h"
static const uint8_t BITMAP_MASK = 1;

struct bitmap{
	uint32_t byte_len;
	uint8_t *bits;
};
void bitmap_init(struct bitmap *bmap);
void bitmap_set(struct bitmap *bmap, uint32_t bit_idx, int8_t value);
int bitmap_scan(struct bitmap *bmap, uint32_t len);

#endif
