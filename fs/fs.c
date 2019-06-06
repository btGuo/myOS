#include "fs.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "global.h"
#include "ide.h"
#include "bitmap.h"
#include "string.h"
#include "memory.h"

struct partition *cur_par = NULL;
extern uint8_t channel_cnt;
extern struct ide_channel channels[2];

inline void write_block(struct parttition *part, struct buffer_head *bh){
	buffer_write(&part->buffer, bh); 
}

inline struct buffer_head *read_block(struct parttition *part, uint32_t blk_nr){
	return buffer_read(&part->buffer, blk_nr);
}

inline void write_indirect(struct parttition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	ide_write(part->disk, sta_blk_nr, data, cnt);
}	

inline void read_indirect(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	ide_read(part->disk, sta_blk_nr, data, cnt);
}

static void partition_format(struct partition *part){
	printk("partition %s start format...\n", part->name);

	//总块数
	uint32_t blocks = part->sec_cnt / BLK_PER_SEC;
	//组数
	uint32_t groups_cnt = blocks / GROUP_BLKS;
	part->groups_cnt = groups_cnt;
	//剩余块数
	uint32_t res_blks = blocks % GROUP_BLKS; 

	uint32_t h_blks = DIV_ROUND_UP(sizeof(struct super_block) 
			+ sizeof(struct group) * groups_cnt, BLOCK_SIZE);
	uint32_t size = h_blks * BLOCK_SIZE;

	uint8_t *buf = sys_malloc(size);
	memset(buf, 0, size);

//初始化超级块
	struct super_block *sb = (struct super_block *)buf;
 	
	sb->magic = SUPER_MAGIC;
	sb->major_rev = MAJOR;
	sb->minor_rev = MINOR;

	sb->block_size = BLOCK_SIZE;
	sb->blocks_per_group = GROUP_BLKS;
	sb->inodes_per_group = INODES_PER_GROUP;
	sb->res_blocks = res_blks;

	sb->blocks_count = blocks;
	sb->inodes_count = INODES_PER_GROUP * groups_cnt;
	sb->free_blocks_count = sb->blocks_count - 1;
	sb->free_inodes_count = sb->inodes_count;

	sb->groups_table = 1;
//初始化块组
	struct group *gp = (struct group *)(buf + sizeof(struct super_block));
	struct group *gp_end = gp + groups_cnt;
	uint32_t used_blks = 1 + h_blks + BLOCKS_BMP_BLKS + INODES_BLKS + INODES_BLKS;

	while(gp != gp_end){
		gp->free_blocks_count = sb->blocks_per_group - used_blks;
		gp->free_inodes_count = sb->inodes_per_group;
		gp->block_bitmap = BLOCKS(sb, BLOCKS_BMP_BLKS);
		gp->inode_bitmap = BLOCKS(sb, INODES_BMP_BLKS);
		gp->inode_table  = BLOCKS(sb, INODES_BLKS);
		++gp;
	}
	
	uint32_t cnt = 0;
	while(cnt < groups_cnt){
		write_blocks(part, GROUP_BLK(sb, cnt), h_blks, buf);
		++cnt;
	}

	gp = (struct group *)(buf + sizeof(struct super_block));
	struct bitmap bitmap;
	bitmap->byte_len = GROUP_BLKS / 8;
	bitmap->bits = (uint8_t *)sys_malloc(bitmap->byte_len);
	bitmap_set_range(&bitmap, 0, 0, used_blks);
	cnt = 0;
	while(cnt < groups_cnt){
	       write_blocks(part, GROUP_BLK(sb, cnt) + gp->block_bitmap, 1);
	}

}


static void mount_partition(struct partition *part){

	printk("mount_partition in %s\n", part->name);
	cur_par = part;
//读取超级块和块组描述符
	uint32_t h_blks = DIV_ROUND_UP(sizeof(struct super_block) 
			+ sizeof(struct group) * groups_cnt, BLOCK_SIZE);
	uint32_t size = h_blks * BLOCK_SIZE;
	uint8_t *buf = sys_malloc(size);

	if(!buf){
		PANIC("no more space\n");
	}
	part->sb = (struct super_block *)buf;
	part->groups = (struct group *)(buf + sizeof(struct super_block));

	read_indirect(part, 1, buf, h_blks);

/*
	struct super_block *sb = part->sb;
	struct bitmap *bmp = &part->block_bitmap;

//读取block位图
	bmp->byte_len = sb->block_bitmap_sects * SECTOR_SIZE;
       	bmp->bits = sys_malloc(bmp->byte_len);	

	if(!bmp->bits){
		PANIC("no more space\n");
	}
	ide_read(part->disk, sb->block_bitmap_lba, bmp->bits, sb->block_bitmap_sects);

//读取inode 位图
 	bmp = &part->inode_bitmap;
	bmp->byte_len = sb->inode_bitmap_sects * SECTOR_SIZE;
	bmp->bits = sys_malloc(bmp->byte_len);
	
	if(!bmp->bits){
		PANIC("no more space\n");
	}
	ide_read(part->disk, sb->inode_bitmap_lba, bmp->bits, sb->inode_bitmap_sects);
*/
}

void filesys_init(){

	printk("filesys_init start\n");
	char *default_part = "sdb1";
	int cno = 0;
	int dno = 0;
	int pno = 0;
	struct partition *part = NULL;
	struct disk *hd = NULL;
	struct super_block sb;
	//遍历通道
	while(cno < channel_cnt){
		dno = 0;
		//遍历磁盘
		while(dno < 2){
		//1号盘不处理	
			if(dno == 0){
				++dno; continue;
			}

			hd = &channels[cno].devices[dno];
			part = hd->prim_parts;
			pno = 0;
			//遍历分区
			while(pno < 12){
				if(pno == 4){
					part = hd->logic_parts;
				}

				//分区存在
				if(part->sec_cnt){
					memset(&sb, 0, SECTOR_SIZE);
					ide_read(hd, part->start_lba + 1, &sb, 1);
					//验证魔数
					if(sb.magic == SUPER_MAGIC){
					}else {
						partition_format(part);
					}
					//如果是默认分区，则挂载
					if(!strcmp(part->name, default_part)){
					       mount_partition(part);
					}	       
				}
				++pno;
				++part;
			}
			++dno;
		}
		++cno;
	}
	
	printk("filesys_init done\n");
}

