
#include <stdint.h>
#include <stdbool.h>

struct md5_ctx {
	uint32_t res[4];
	uint8_t *buf;
};

void md5_init(struct md5_ctx *md5);
void md5_update(struct md5_ctx *md5, const char *data, uint32_t size);
void md5_print(struct md5_ctx *md5);
bool md5_check(struct md5_ctx *src, const char *data);


