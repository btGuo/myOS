#include"bitmap.h"
#include"debug.h"
#include"string.h"

void bitmap_init(struct bitmap *bmap){
	memset(bmap->bits, 0, bmap->byte_len);
}

int bitmap_scan(struct bitmap *bmap, uint32_t len){
	uint32_t idx_byte = 0;
	while((bmap->bits[idx_byte] == 0xff) && (idx_byte < bmap->byte_len))
		++idx_byte;

	if(idx_byte == bmap->byte_len)
		return -1;
	
	uint32_t cnt = 0;
	uint32_t i = idx_byte * 8;
	uint32_t start = i;
	uint32_t max_len = bmap->byte_len * 8;
	uint8_t  mask = BITMAP_MASK;

	for(; i < max_len; ++i){
		if(!mask)
			mask = BITMAP_MASK;
		if(!(bmap->bits[i/8] & mask)){
			++cnt;
			if(cnt == len)
				return start;
		}else{
			cnt = 0;
			start = i + 1;
		}
	}
	return -1;
}

void bitmap_set(struct bitmap *bmap, uint32_t bit_idx, int8_t value){
	ASSERT(value == 0 || value == 1);
	uint8_t mask = BITMAP_MASK;
	mask <<= bit_idx % 8;
	if(value)
		bmap->bits[bit_idx/8] |= mask;
	else 
		bmap->bits[bit_idx/8] &= ~mask;
}
	


