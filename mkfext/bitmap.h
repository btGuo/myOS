#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H

#include "local.h"

struct bitmap{
	uint32_t byte_len;
	uint8_t *bits;
};
void bitmap_init(struct bitmap *bmap);
void bitmap_set(struct bitmap *bmap, uint32_t bit_idx, int8_t value);
void bitmap_set_range(struct bitmap *bmap, uint32_t bit_idx_start, int8_t value, int32_t cnt);
int bitmap_scan(struct bitmap *bmap, uint32_t len);
uint8_t bitmap_verify(struct bitmap *bmap, uint32_t idx);
void bitmap_toString(struct bitmap *bmap);

#endif
