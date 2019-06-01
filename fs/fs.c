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

static void partition_format(struct partition *part){
	printk("partition %s start format...\n", part->name);

	uint32_t inode_secs = DIV_ROUND_UP(sizeof(struct inode) * MAX_FILES_PER_PART,
		       	SECTOR_SIZE);
	//先统计出字节数, 再统计扇区数
	uint32_t inode_bmp_secs = DIV_ROUND_UP(MAX_FILES_PER_PART, 8);
		 inode_bmp_secs = DIV_ROUND_UP(inode_bmp_secs, SECTOR_SIZE);
	
	uint32_t free_secs = part->sec_cnt - 2 - inode_bmp_secs - inode_secs;
	
	//上取整
	uint32_t block_bmp_secs = DIV_ROUND_UP(free_secs, (BITS_PER_SEC / SEC_PER_BLK + 1));
	//剩下的全给块
	uint32_t block_secs = free_secs - block_bmp_secs;
	//剩下的位数，有多余的位
	uint32_t res_bits = block_bmp_secs * SECTOR_SIZE * 8 - block_secs;

	printk("     total secs: %d   ", part->sec_cnt);
	printk("     block_bmp_secs: %d   ", block_bmp_secs);
	printk("    block_secs : %d   ", block_secs);
	printk("    inode_bmp_secs : %d    ", inode_bmp_secs);
	printk("    inode_secs : %d    ", inode_secs);
	printk("    res_bits : %d\n", res_bits);
	
	struct super_block sb;
	sb.magic = SUPER_MAGIC;
	sb.sec_cnt = part->sec_cnt;
	sb.inode_cnt = MAX_FILES_PER_PART;
	sb.part_lba_base = part->start_lba;

	sb.block_bitmap_lba = sb.part_lba_base + 2;
	sb.block_bitmap_sects = block_bmp_secs;

	sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
	sb.inode_bitmap_sects = inode_bmp_secs;

	sb.inode_table_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
	sb.inode_table_sects = inode_secs;

	sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
	sb.root_inode_no = 0;
	sb.dir_entry_size = sizeof(struct dir_entry);

	struct disk *hd = part->disk;
	ide_write(hd, part->start_lba + 1, &sb, 1);

	uint32_t buf_size = (block_bmp_secs > inode_secs ?  block_bmp_secs 
		:inode_secs) * SECTOR_SIZE; 
	uint8_t *buf = (uint8_t *)sys_malloc(buf_size);
	buf[0] |= 1;

	//创建位图以便处理
	struct bitmap bmp;
//先处理块位图
	bmp.byte_len = block_bmp_secs * SECTOR_SIZE;
	bmp.bits = buf;
	//剩下没用的位全部置1
	bitmap_set_range(&bmp, block_secs, 1, res_bits);
	ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

//处理i节点位图
	memset(buf, 0, buf_size);
	buf[0] |= 1;
	ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

//处理i节点表
	buf[0] = 0;
	struct inode *i = (struct inode *)buf;
	//根目录两个目录项
	i->i_size = sb.dir_entry_size * 2;
	i->i_no = 0;
	i->i_sectors[0] = sb.data_start_lba;
	ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);
//处理块
	memset(buf, 0, sb.dir_entry_size * 2);
	struct dir_entry *p_de = (struct dir_entry *)buf;
	//分别处理两个目录
	memcpy(p_de->filename, ".", 1);
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;
	++p_de;

	memcpy(p_de->filename, "..", 2);
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;

	ide_write(hd, sb.data_start_lba, buf, 1);

	sys_free(buf);
}

static void mount_partition(struct partition *part){

	printk("mount_partition in %s\n", part->name);
	cur_par = part;
//读取超级块
	part->sb = (struct super_block *)sys_malloc(sizeof(struct super_block));
	if(!part->sb){
		PANIC("no more space\n");
	}
	ide_read(part->disk, part->start_lba + 1, part->sb, 1);

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


