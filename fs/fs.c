#include "ide.h"
#include "fs.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "global.h"
#include "bitmap.h"
#include "string.h"
#include "memory.h"
#include "group.h"
#include "buffer.h"

struct partition *cur_par = NULL;
extern uint8_t channel_cnt;
extern struct ide_channel channels[2];

/**
 * 路径解析，提取出/之间的名字
 *
 * @param path 路径名
 * @param res  遍历结果
 *
 * @return 遍历完当前项后path的位置
 * 	@retval NULL 没有东西可以提取了
 */
static char *path_parse(char *path, char *res){

	while(*path == '/')
		++path;
	
	while(*path != '/' && *path != '\0')
		*res++ = *path++;

	*res = '\0';
	if(!*path)
		return NULL;
	
	return path;
}

/**
 * 将路径分割成文件名和目录名
 *
 * @param path 路径名
 * @param filename 文件名
 * @param dirname 目录名
 *
 * @note 对于/a/b/c 分割结果为 filename : c  dirname /a/b/
 */
static void split_path(char *path, char *filename, char *dirname){
	char *pos = path;
	char *head = path;
	while(*path){
		if(*path == '/')
			pos = path + 1;
		++path;
	}

	while(head != pos) 
		*dirname++ = *head++;
	*dirname = '\0';

	while(pos != path)
		*filename++ = *pos++;
	*filename = '\0';
}


/**
 * @brief 写磁盘块
 * @note 操作的对象为buffer_head
 */
void write_block(struct partition *part, struct buffer_head *bh){
	if(bh->is_buffered){
		BUFW_BLOCK(bh);
		return;
	}
	ide_write(part->disk, bh->blk_nr, bh->data, 1);
}


/**
 * @brief 读取磁盘块
 * @detail 先在缓冲区中查找，命中则直接返回，否则从磁盘读取块并尝试加入
 * 缓冲区
 *
 * @param blk_nr 块号
 */
struct buffer_head *read_block(struct partition *part, uint32_t blk_nr){
	struct buffer_head *bh = buffer_read_block(&part->io_buffer, blk_nr);
	if(bh){
		return bh;
	}
	ALLOC_BH(bh);
	bh->blk_nr = blk_nr;
	bh->lock = true;
	bh->is_buffered = true;
	//从磁盘读
	ide_read(part->disk, blk_nr, bh->data, 1);
	//加入缓冲区
	if(!buffer_add_block(&part->io_buffer, bh)){
		//缓冲区已经满了
		bh->is_buffered = false;
	}
	return bh;
}


/**
 * 释放内存中的块
 */
void release_block(struct buffer_head *bh){
	if(bh->is_buffered){
		BUFR_BLOCK(bh);
		return;
	}
	sys_free(bh->data);
	sys_free(bh);
}

/** 下面这两个好像没什么意义 */

/**
 * 磁盘直接写
 */
void write_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	ide_write(part->disk, sta_blk_nr, data, cnt);
}	

/**
 * 磁盘直接读
 */
void read_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	ide_read(part->disk, sta_blk_nr, data, cnt);
}

/**
 * 分区格式化
 */

static void partition_format(struct partition *part){
	printk("partition %s start format...\n", part->name);

	//总块数
	uint32_t blocks = part->sec_cnt / BLK_PER_SEC;
	//组数
	uint32_t groups_cnt = blocks / GROUP_BLKS;
	part->groups_cnt = groups_cnt;
	//剩余块数
	uint32_t res_blks = blocks % GROUP_BLKS; 


	//超级块加上块组大小
	uint32_t h_blks = DIV_ROUND_UP(sizeof(struct super_block) 
			+ sizeof(struct group) * groups_cnt, BLOCK_SIZE);
	uint32_t size = h_blks * BLOCK_SIZE;

	uint8_t *buf = sys_malloc(size);
	memset(buf, 0, size);

	//每组已使用的块: 超级块和块组描述符，块位图，inode位图，inode表
	uint32_t used_blks = h_blks + BLOCKS_BMP_BLKS + INODES_BMP_BLKS + INODES_BLKS;
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
	sb->free_blocks_count = sb->blocks_count - 1 - used_blks * groups_cnt;
	sb->free_inodes_count = sb->inodes_count - 1;

	sb->groups_table = 1;
//初始化块组
	struct group *gp_head = (struct group *)(buf + sizeof(struct super_block));
	struct group *gp = gp_head;
	uint32_t cnt = groups_cnt;
	
	//group初始化
	while(cnt--){
		gp->free_blocks_count = sb->blocks_per_group - h_blks;
		gp->free_inodes_count = sb->inodes_per_group;
		gp->block_bitmap = GROUP_INNER(gp, BLOCKS_BMP_BLKS, cnt);
		gp->inode_bitmap = GROUP_INNER(gp, INODES_BMP_BLKS, cnt);
		gp->inode_table  = GROUP_INNER(gp, INODES_BLKS, cnt);
		++gp;
	}
	
	gp = gp_head;
	cnt = 0;

	//直接写入磁盘
	while(cnt < groups_cnt){
		write_direct(part, GROUP_BLK(sb, cnt), buf, h_blks);
		++cnt;
	}

//初始化块组内块位图
	gp = gp_head;
	struct bitmap bitmap;
	bitmap.byte_len = GROUP_BLKS / 8;
	bitmap.bits = (uint8_t *)sys_malloc(bitmap.byte_len);
	bitmap_set_range(&bitmap, 0, 0, used_blks);
	cnt = 0;
	while(cnt < groups_cnt){
	       write_direct(part, GROUP_BLK(sb, cnt) + gp->block_bitmap, bitmap.bits, 1);
	}
	sys_free(buf);
}

/**
 * 挂载分区，即将分区加载到内存
 */

static void mount_partition(struct partition *part){

	printk("mount_partition in %s\n", part->name);
	cur_par = part;

	part->groups = sys_malloc(part->groups_cnt * sizeof(struct group_info));
	part->sb = sys_malloc(sizeof(struct super_block));
	struct group_info *gp_info = part->groups;
	memset(gp_info, 0, part->groups_cnt * sizeof(struct group_info));
//读取超级块和块组描述符
	uint32_t h_blks = DIV_ROUND_UP(sizeof(struct super_block) 
			+ sizeof(struct group) * part->groups_cnt, BLOCK_SIZE);

	uint32_t size = h_blks * BLOCK_SIZE;
	uint8_t *buf = sys_malloc(size);
	struct supeer_block *sb = (struct super_block *)buf;
	struct group *gp = (struct group *)(buf + sizeof(struct super_block));

	MEMORY_OK(buf);
	//直接读取
	read_direct(part, 1, buf, h_blks);

	memcpy(part->sb, sb, sizeof(struct super_block));
	uint32_t cnt = part->groups_cnt;
	struct group_info *next = NULL;
	while(cnt--){
		memcpy(gp_info, gp, sizeof(struct group));
		gp_info->next = next;
		next = gp_info;
		++gp_info;
		++gp;
	}

	sys_free(buf);
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

//格式化所有分区
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
//打开根目录
	open_root_dir(cur_par);

	uint32_t fd_idx = 0;
	while(fd_idx < MAX_FILE_OPEN){
		file_table[fd_idx++].fd_inode = NULL;
	}

	printk("filesys_init done\n");
}

/**
 * 提取路径中最后一项目录
 *
 * @param path 目录路径，形式为 /a/b/c/ 最后一个字符是/
 * @note 对于/a/b/c/ 返回c
 */
static struct dir *get_last_dir(const char *path){
	
	//根目录直接返回
	if(!strcmp(path, "/") || !strcmp(path, "/.") || !(path, "/..")){
		return root_dir;
	}

	char name[MAX_FILE_NAME_LEN];
	struct par_dir = root_dir;
	struct dir_entry dir_e;
	
	while(path = path_parse(path, name)){
		if(search_dir_entry(cur_par, par_dir, name, &dir_e)){

			dir_close(par_dir);
			if(dir_e.f_type != FT_DIRECTORY){
				printk("%s is not a directory!\n", name);
				return NULL;
			}
			par_dir = dir_open(cur_par, dir_e.i_no);
		}
		else{
			printk("%s is not found!\n", name);
			return NULL;
		}
	}
	return par_dir;
}

#define MAX_PATH_LEN 128

int32_t sys_open(const char *path, uint8_t flags){
	
	if(path[strlen(path) - 1] == '/'){
		printk("can't open a directory %s\n", path);
		return -1;
	}

	ASSERT(flags <= 7);
	int32_t fd = -1;
	char filename[MAX_FILE_NAME_LEN];
	char dirname[MAX_PATH_LEN];
	struct dir_entry dir_e;

	split_path(path, filename, dirname);
	struct dir *par_dir = get_last_dir(dirname);
	if(!par_dir){
		printk("open directory error\n");
		return -1;
	}

	bool found = search_dir_entry(cur_par, par_dir, filename, &dir_e);

	if(!found && !(flags & O_CREAT)){

		printk("file %s isn't exist\n", filename);
		dir_close(par_dir);
		return -1;

	}else if(found && (flags & O_CREAT)){

		printk("file %s has already exist!\n", filename);
		dir_close(par_dir);
		return -1;
	}
	if(flags & O_CREAT){

		printk("create file %s\n", filename);
		fd = file_create(par_dir, filename, flags);
		dir_close(par_dir);
	}else {
		fd = file_open(dir_e.i_no, flags);
	}
	return fd;
}	

