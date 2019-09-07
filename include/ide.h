#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include "stdint.h"
#include "list.h"
#include "sync.h"
#include "bitmap.h"
#include "hash_table.h"
#include "buffer.h"

typedef unsigned int dev_t;

#define IDE_MAJOR 6
#define MAJOR_SHIFT 16
#define MINOR_SHIFT 4

/**
 * @brief 分区描述符，对应硬盘上的分区，一个硬盘可以有多个分区
 */
struct partition{
	uint32_t start_lba;   ///< 分区起始地址
	uint32_t sec_cnt;     ///< 分区总扇区数
	struct disk *disk;    ///< 该分区所在磁盘
	char name[8];     ///< 分区名字
	dev_t dev_nr;     ///< 设备号
};

/**
 * @brief 磁盘描述符
 */
struct disk{
	char name[8];       ///< 磁盘名字
	struct ide_channel *channel;   ///< 磁盘所在的通道
	struct partition prim_parts[4];  ///< 4个主分区
	struct partition logic_parts[8]; ///< 8个逻辑分区，这里设了上限
	uint8_t dev_no;   ///< 设备号
};

/**
 * @brief ide通道，每条通道可有两个硬盘
 */
struct ide_channel{
	char name[8];   ///< 通道名字
	uint16_t port_base;  ///< 通道io端口
	uint8_t irq_no;     ///< 中断号
	struct mutex_lock lock;   ///< 锁
	struct disk devices[2];  ///< 两个磁盘

};

#define MAX_PARTS 64  ///< 最大分区数
extern struct partition *parts[MAX_PARTS];  ///< 所有分区
extern struct ide_channel channels[2];  ///< 两条ide通道

struct partition *name2part(const char *device);
void write_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);
void read_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);

void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_init();
#endif
