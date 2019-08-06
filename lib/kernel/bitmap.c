#include"bitmap.h"
#include"debug.h"
#include"string.h"

/**
 * 初始化
 */
void bitmap_init(struct bitmap *bmap){
	memset(bmap->bits, 0, bmap->byte_len);
}

/**
 * 检测位图中第idx位的值
 *
 * @return 该位的值 1或者0
 */
uint8_t bitmap_verify(struct bitmap *bmap, uint32_t idx){
	
	ASSERT(idx < bmap->byte_len * 8);
	uint8_t mask = 1;
	mask <<= idx % 8;
	return (bmap->bits[idx / 8] & mask);
}

/**
 * 在位图中查找连续len个空位
 *
 * @return 找到的位置的起点
 * 	@retval -1 没有满足条件的位
 */
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
	uint8_t  mask = 1;

	for(; i < max_len; ++i){
		if(!mask)
			mask = 1;
		if(!(bmap->bits[i/8] & mask)){
			++cnt;
			if(cnt == len)
				return start;
		}else{
			cnt = 0;
			start = i + 1;
		}
		mask <<= 1;
	}
	return -1;
}

/**
 * 在位图中将bit_idx为置为 0 或者 1
 */
void bitmap_set(struct bitmap *bmap, uint32_t bit_idx, int8_t value){
	ASSERT(value == 0 || value == 1);
	uint8_t mask = 1;
	mask <<= bit_idx % 8;
	if(value)
		bmap->bits[bit_idx / 8] |= mask;
	else 
		bmap->bits[bit_idx / 8] &= ~mask;
}

/**
 * 在位图中以bit_idx_start 为起点，连续cnt个值设置为 0 或者 1
 *
 * @attention 参数的可能不太合理
 */
void bitmap_set_range(struct bitmap *bmap, uint32_t bit_idx_start, \
		int8_t value, int32_t cnt){
	//已经遍历的位
	int size = 0;
	while(1){
		if(!((bit_idx_start + size)% 8))
			break;
		bitmap_set(bmap, (bit_idx_start + size), value);
		++size;
	}

	//减去前面以遍历位数再对8取整
	int bytes = (cnt - size) / 8;
	int offset = bit_idx_start / 8 + (bit_idx_start % 8 ? 1 : 0);
	
	//批量处理
	if(value == 0)
		memset(bmap->bits + offset, 0, bytes);
	else
		memset(bmap->bits + offset, 0xff, bytes);

	//size 加上字节数
	size += bytes * 8;

	while(size < cnt){
		bitmap_set(bmap, bit_idx_start + size, value);
		++size;
	}
}

/**
 * 打印位图二进制模式
 */
void bitmap_toString(struct bitmap *bmap){
	uint8_t mask = 0x01;
	int size = bmap->byte_len * 8;
	int i = 0;
	for(i = 0; i < size; ++i){
		if(i % 8 == 0)
			printf(" ");
		if(i % 80 == 0)
			printf("\n");

		if((bmap->bits[i / 8] & mask))
			printf("1");
		else 
			printf("0");

		if(mask & 0x80)
			mask = 0x01;
		else	
			mask <<= 1;
	}
	printf("\n");
}

