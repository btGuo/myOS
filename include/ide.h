#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include "stdint.h"
#include "list.h"
#include "sync.h"
#include "bitmap.h"
#include "hash_table.h"

/**
 * @brief 分区描述符，对应硬盘上的分区，一个硬盘可以有多个分区
 */
struct partition{
	uint32_t start_lba;   ///< 分区起始地址
	uint32_t sec_cnt;     ///< 分区总扇区数
	struct disk *disk;    ///< 该分区所在磁盘
	struct list_head part_tag;  
	char name[8];     ///< 分区名字
	struct super_block *sb;    ///< 分区超级块
	struct bitmap block_bitmap;   ///< 块位图
	struct bitmap inode_bitmap;   ///< inode位图
	struct list_head open_inodes;   ///< inode缓存队列
};

/**
 * @brief 磁盘描述符
 */
struct disk{
	char name[8];       ///< 磁盘名字
	struct ide_channel *channel;   ///< 磁盘所在的通道
	struct partition prim_parts[4];  ///< 4个主分区
	struct partition logic_parts[8]; ///< 8个逻辑分区，这里设了上限
	struct hash_table io_buffer;     ///< 缓冲区，保留文件系统块指针
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

//	bool expecting_intr;   
//	struct semaphore disk_done;
};

void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_init();
#endif
