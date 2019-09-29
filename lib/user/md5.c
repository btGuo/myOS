#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "md5.h"

#define A 0x67452301
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476

#define S11 7
#define S12 12
#define S13 17
#define S14 22

#define S21 5
#define S22 9
#define S23 14
#define S24 20

#define S31 4
#define S32 11
#define S33 16
#define S34 23

#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

/**
 * 循环左移
 */
static inline uint32_t rotate_left(uint32_t x, uint32_t n)
{
	return ((x << n) | (x >> (32 - n)));
}

static inline uint32_t FF(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
			  uint32_t m, uint32_t s, uint32_t t)
{

	return b + rotate_left((a + F(b, c, d) + m + t), s);
}

static inline uint32_t GG(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
			  uint32_t m, uint32_t s, uint32_t t)
{

	return b + rotate_left((a + G(b, c, d) + m + t), s);
}

static inline uint32_t HH(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
			  uint32_t m, uint32_t s, uint32_t t)
{

	return b + rotate_left((a + H(b, c, d) + m + t), s);
}

static inline uint32_t II(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
			  uint32_t m, uint32_t s, uint32_t t)
{

	return b + rotate_left((a + I(b, c, d) + m + t), s);
}

/**
 * 初始化md5
 */
void md5_init(struct md5_ctx *md5)
{

	md5->res[0] = A;
	md5->res[1] = B;
	md5->res[2] = C;
	md5->res[3] = D;
}

static void md5_cyc(struct md5_ctx *md5, uint32_t * x)
{

	uint32_t a = md5->res[0];
	uint32_t b = md5->res[1];
	uint32_t c = md5->res[2];
	uint32_t d = md5->res[3];
	
        a = FF(a, b, c, d, x[0], S11, 0xd76aa478L); /* 1 */
        d = FF(d, a, b, c, x[1], S12, 0xe8c7b756L); /* 2 */
        c = FF(c, d, a, b, x[2], S13, 0x242070dbL); /* 3 */
        b = FF(b, c, d, a, x[3], S14, 0xc1bdceeeL); /* 4 */
        a = FF(a, b, c, d, x[4], S11, 0xf57c0fafL); /* 5 */
        d = FF(d, a, b, c, x[5], S12, 0x4787c62aL); /* 6 */
        c = FF(c, d, a, b, x[6], S13, 0xa8304613L); /* 7 */
        b = FF(b, c, d, a, x[7], S14, 0xfd469501L); /* 8 */
        a = FF(a, b, c, d, x[8], S11, 0x698098d8L); /* 9 */
        d = FF(d, a, b, c, x[9], S12, 0x8b44f7afL); /* 10 */
        c = FF(c, d, a, b, x[10], S13, 0xffff5bb1L); /* 11 */
        b = FF(b, c, d, a, x[11], S14, 0x895cd7beL); /* 12 */
        a = FF(a, b, c, d, x[12], S11, 0x6b901122L); /* 13 */
        d = FF(d, a, b, c, x[13], S12, 0xfd987193L); /* 14 */
        c = FF(c, d, a, b, x[14], S13, 0xa679438eL); /* 15 */
        b = FF(b, c, d, a, x[15], S14, 0x49b40821L); /* 16 */

        /*第二轮*/
        a = GG(a, b, c, d, x[1], S21, 0xf61e2562L); /* 17 */
        d = GG(d, a, b, c, x[6], S22, 0xc040b340L); /* 18 */
        c = GG(c, d, a, b, x[11], S23, 0x265e5a51L); /* 19 */
        b = GG(b, c, d, a, x[0], S24, 0xe9b6c7aaL); /* 20 */
        a = GG(a, b, c, d, x[5], S21, 0xd62f105dL); /* 21 */
        d = GG(d, a, b, c, x[10], S22, 0x2441453L); /* 22 */
        c = GG(c, d, a, b, x[15], S23, 0xd8a1e681L); /* 23 */
        b = GG(b, c, d, a, x[4], S24, 0xe7d3fbc8L); /* 24 */
        a = GG(a, b, c, d, x[9], S21, 0x21e1cde6L); /* 25 */
        d = GG(d, a, b, c, x[14], S22, 0xc33707d6L); /* 26 */
        c = GG(c, d, a, b, x[3], S23, 0xf4d50d87L); /* 27 */
        b = GG(b, c, d, a, x[8], S24, 0x455a14edL); /* 28 */
        a = GG(a, b, c, d, x[13], S21, 0xa9e3e905L); /* 29 */
        d = GG(d, a, b, c, x[2], S22, 0xfcefa3f8L); /* 30 */
        c = GG(c, d, a, b, x[7], S23, 0x676f02d9L); /* 31 */
        b = GG(b, c, d, a, x[12], S24, 0x8d2a4c8aL); /* 32 */

        /*第三轮*/
        a = HH(a, b, c, d, x[5], S31, 0xfffa3942L); /* 33 */
        d = HH(d, a, b, c, x[8], S32, 0x8771f681L); /* 34 */
        c = HH(c, d, a, b, x[11], S33, 0x6d9d6122L); /* 35 */
        b = HH(b, c, d, a, x[14], S34, 0xfde5380cL); /* 36 */
        a = HH(a, b, c, d, x[1], S31, 0xa4beea44L); /* 37 */
        d = HH(d, a, b, c, x[4], S32, 0x4bdecfa9L); /* 38 */
        c = HH(c, d, a, b, x[7], S33, 0xf6bb4b60L); /* 39 */
        b = HH(b, c, d, a, x[10], S34, 0xbebfbc70L); /* 40 */
        a = HH(a, b, c, d, x[13], S31, 0x289b7ec6L); /* 41 */
        d = HH(d, a, b, c, x[0], S32, 0xeaa127faL); /* 42 */
        c = HH(c, d, a, b, x[3], S33, 0xd4ef3085L); /* 43 */
        b = HH(b, c, d, a, x[6], S34, 0x4881d05L); /* 44 */
        a = HH(a, b, c, d, x[9], S31, 0xd9d4d039L); /* 45 */
        d = HH(d, a, b, c, x[12], S32, 0xe6db99e5L); /* 46 */
        c = HH(c, d, a, b, x[15], S33, 0x1fa27cf8L); /* 47 */
        b = HH(b, c, d, a, x[2], S34, 0xc4ac5665L); /* 48 */

        /*第四轮*/
        a = II(a, b, c, d, x[0], S41, 0xf4292244L); /* 49 */
        d = II(d, a, b, c, x[7], S42, 0x432aff97L); /* 50 */
        c = II(c, d, a, b, x[14], S43, 0xab9423a7L); /* 51 */
        b = II(b, c, d, a, x[5], S44, 0xfc93a039L); /* 52 */
        a = II(a, b, c, d, x[12], S41, 0x655b59c3L); /* 53 */
        d = II(d, a, b, c, x[3], S42, 0x8f0ccc92L); /* 54 */
        c = II(c, d, a, b, x[10], S43, 0xffeff47dL); /* 55 */
        b = II(b, c, d, a, x[1], S44, 0x85845dd1L); /* 56 */
        a = II(a, b, c, d, x[8], S41, 0x6fa87e4fL); /* 57 */
        d = II(d, a, b, c, x[15], S42, 0xfe2ce6e0L); /* 58 */
        c = II(c, d, a, b, x[6], S43, 0xa3014314L); /* 59 */
        b = II(b, c, d, a, x[13], S44, 0x4e0811a1L); /* 60 */
        a = II(a, b, c, d, x[4], S41, 0xf7537e82L); /* 61 */
        d = II(d, a, b, c, x[11], S42, 0xbd3af235L); /* 62 */
        c = II(c, d, a, b, x[2], S43, 0x2ad7d2bbL); /* 63 */
        b = II(b, c, d, a, x[9], S44, 0xeb86d391L); /* 64 */

	md5->res[0] += a;
	md5->res[1] += b;
	md5->res[2] += c;
	md5->res[3] += d;
}

/**
 * 计算md5
 */
void md5_update(struct md5_ctx *md5, const char *data, uint32_t size)
{
	uint32_t sz = size;
	while (sz >= 64) {

		md5_cyc(md5, (uint32_t *) data);
		data += 64;
		sz -= 64;
	}

	uint32_t pad_size = (sz + 8 + 63) & (~63);

	uint8_t *buf = malloc(pad_size);
	uint8_t *head = buf;
	memset(buf, 0, pad_size);
	memcpy(buf, data, sz);
	buf[sz] = 0x80;

	uint64_t *p = (uint64_t *) (buf + pad_size - 8);
	*p = (uint64_t) (size << 3);

	while (pad_size) {
		md5_cyc(md5, (uint32_t *) buf);
		pad_size -= 64;
		buf += 64;
	}
	free(head);
}

void md5_print(struct md5_ctx *md5)
{

	printk("after encryption:\n");
	for (int i = 0; i < 4; i++) {
		uint8_t *p = (uint8_t *) & md5->res[i];
		printk("%x", *(p + 0));
		printk("%x", *(p + 1));
		printk("%x", *(p + 2));
		printk("%x", *(p + 3));
	}
	printk("\n");
}

bool md5_check(struct md5_ctx *src, const char *data){

	struct md5_ctx md5;
	md5_init(&md5);
	md5_update(&md5, data, strlen(data));

	for(int i = 0; i < 4; i++){
		if(md5.res[i] != src->res[i]){
			return false;
		}
	}
	return true;
}
