#ifdef __TEST
#include <bitmap.h>
#include <kernelio.h>

#define MAX_BYTES 1024
#define BITS (MAX_BYTES * 8)

void test_bitmap(){

	struct bitmap bmp;
	uint8_t data[MAX_BYTES];
	bmp.byte_len = MAX_BYTES;
	bmp.bits = data;
	bitmap_init(&bmp);

	bitmap_toString(&bmp);

	bitmap_set(&bmp, 0, 1);
	bitmap_toString(&bmp);

	bitmap_set_range(&bmp, 100, 1, 300);
	bitmap_toString(&bmp);

	uint32_t idx = bitmap_scan(&bmp, 200);
	printk("start idx : %d\n", idx);

	uint32_t bit = bitmap_verify(&bmp, 150);
	printk("bit : %d\n", bit);
}

#endif
