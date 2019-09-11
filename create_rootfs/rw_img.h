#ifndef __RW_IMG_H
#define __RW_IMG_H
#include <stdint.h>

#define NAME_LEN 32

/**
 * @brief 分区描述符，对应硬盘上的分区，一个硬盘可以有多个分区
 */
struct partition{
	uint32_t start_lba;   ///< 分区起始地址
	uint32_t sec_cnt;     ///< 分区总扇区数
	char name[NAME_LEN];     ///< 分区名字
	struct disk *disk;
};

/**
 * @brief 磁盘描述符
 */
struct disk{
	char name[NAME_LEN];       ///< 磁盘名字
	struct partition prim_parts[4];  ///< 4个主分区
	struct partition logic_parts[8]; ///< 8个逻辑分区，这里设了上限
	uint8_t dev_no;   ///< 设备号
	uint32_t fd;
};

#define MAX_PARTS 12
extern struct partition *parts[MAX_PARTS];  ///< 所有分区信息

void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
int32_t ide_ctor(struct disk *hd, const char *image);
int32_t ide_dtor(struct disk *hd);
void ide_scan(struct disk *hd);

#endif
