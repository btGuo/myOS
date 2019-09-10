#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "rw_img.h"

#include "local.h"

#define MAX_PARTS 12
#define SECTOR_SZ 512

struct partition *parts[MAX_PARTS] = {NULL};  ///< 所有分区信息

/**
 * @brief 分区表描述符 一个32字节
 */
struct partition_table_entry{
	uint8_t bootable;       ///<  是否可引导      
	uint8_t start_head;     ///<  起始磁头号
	uint8_t start_sec;      ///<  起始扇区号
	uint8_t start_chs;      ///<  起始柱面号
	uint8_t fs_type;        ///<  分区类型
	uint8_t end_head;       ///<  结束磁头号
	uint8_t end_sec;        ///<  结束扇区号
	uint8_t end_chs;  	///<  结束柱面号
	uint32_t start_lba;     ///<  起始扇区lba地址
	uint32_t sec_cnt;       ///<  本分区u扇区数量
}__attribute__((packed));

void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt){

	lseek(hd->fd, lba * SECTOR_SZ, SEEK_SET);
	read(hd->fd, buf, cnt * SECTOR_SZ);
}

void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt){

	lseek(hd->fd, lba * SECTOR_SZ, SEEK_SET);
	write(hd->fd, buf, cnt * SECTOR_SZ);
}

/**
 * @brief 引导扇区描述符mbr 或 ebr
 */
struct boot_sector{
	struct partition_table_entry partition_table[4];   ///< 四个分区表项
	uint16_t signature;   ///< 魔数
}__attribute__((packed));

/**
 * 根据分区文件名获取分区
 */
struct partition *name2part(const char *device){
	const char *name = 1 + strrchr(device, '/');
	printk("name %s\n", name);
	uint32_t i = 0;
	for(; i < MAX_PARTS && parts[i]; i++){
		if(strcmp(parts[i]->name, name) == 0){
			return parts[i];
		}
	}
	return NULL;
}

/**
 * 打印分区信息
 */
static void partition_info(struct partition *part){

	printk("%s\n\tstart_lba:%x\n\tsec_cnt:%x\n", 
			part->name, 
			part->start_lba,
			part->sec_cnt
	);
}

/**
 * 读取启动扇区
 */
static void get_boot_sector(struct disk *hd, uint32_t lba, \
		struct boot_sector *bs){
	char sec[512];
	ide_read(hd, lba, sec, 1);
	//复制后66字节
	memcpy(bs, sec + 446, 66);
}

/** 这两个变量为了方便用 */
static uint32_t part_idx = 0;
static uint32_t ext_lba_base = 0; 

/**
 * 获取拓展分区
 */
static void get_ext_partition(struct disk *hd, uint32_t l_no, uint32_t s_lba){
	if(l_no >= 8)
		return;
	struct boot_sector bs;
	struct partition *part = NULL;
	get_boot_sector(hd, s_lba, &bs);
	struct partition_table_entry *p = bs.partition_table;

	if(p->fs_type != 0){
		//添加逻辑分区

		part = &hd->logic_parts[l_no];
		part->start_lba = s_lba + p->start_lba;
		part->sec_cnt = p->sec_cnt;

		parts[part_idx++] = part;
		sprintf(part->name, "%s%d", hd->name, l_no + 5);
		partition_info(part);
	}
	//还有分区
	if(p[1].fs_type != 0)
		get_ext_partition(hd, l_no + 1, ext_lba_base + p[1].start_lba);
}

/**
 * 扫描分区表，设置分区描述符中的前四项
 */
static void partition_scan(struct disk *hd, uint32_t ext_lba){

	struct boot_sector bs;
	get_boot_sector(hd, ext_lba, &bs);
	struct partition_table_entry *p = bs.partition_table;
	struct partition *part = NULL;
	int idx = 0;
	int p_no = 0;
	while(idx++ < 4){
		//总拓展分区
		if(p->fs_type == 0x5){
			ext_lba_base = p->start_lba;
			get_ext_partition(hd, 0, p->start_lba);

		}else if(p->fs_type != 0){
			//主分区
			ASSERT(p_no < 4);
			part = &hd->prim_parts[p_no++];
			part->start_lba = ext_lba + p->start_lba;
			part->sec_cnt = p->sec_cnt;

			parts[part_idx++] = part;
			sprintf(part->name, "%s%d", hd->name, p_no);
			partition_info(part);
		}
		++p;
	}
}

int32_t ide_ctor(struct disk *hd, const char *image){

	strcpy(hd->name, image);
	if((hd->fd = open(image, O_RDWR)) == -1){
		printk("open file %s error\n", image);
		return -1;
	}
	return 0;
}

void ide_scan(struct disk *hd){
	partition_scan(hd, 0);
}


int32_t ide_dtor(struct disk *hd){

	close(hd->fd);
}
