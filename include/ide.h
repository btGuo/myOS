#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include "stdint.h"
#include "list.h"
#include "sync.h"
#include "bitmap.h"

struct partition{
	uint32_t start_lba;
	uint32_t sec_cnt;
	struct disk *disk;
	struct list_head part_tag;
	char name[8];
	struct super_block *sb;
	struct bitmap block_bitmap;
	struct bitmap inode_bitmap;
	struct list_head open_inodes;
};

struct disk{
	char name[8];
	struct ide_channel *channel;
	struct partition prim_parts[4];
	struct partition logic_parts[8];
	uint8_t dev_no;
};

struct ide_channel{
	char name[8];
	uint16_t port_base;
	uint8_t irq_no;
	struct mutex_lock lock;
	bool expecting_intr;
	struct semaphore disk_done;
	struct disk devices[2];
};

void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt);
void ide_init();
#endif
