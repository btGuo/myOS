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
#include "file.h"
#include "debug.h"
#include "thread.h"
#include "block.h"
#include "inode.h"

/**
 * TODO
 * fs 下的函数都带有partition分区指针，用来指出在那个分区上操作，应该改为
 * 文件系统的指针，fs下相关的操作应该与分区无关。
 */

//TODO 检查内存块复用
//添加目录项缓冲

const char *default_part = "sdb1";
struct partition *cur_par = NULL; ///< 当前活跃分区
extern uint8_t channel_cnt;
extern struct ide_channel channels[2];
extern struct dir root_dir;
extern struct file file_table[MAX_FILE_OPEN];
extern struct task_struct *curr;


//==========================================================================
//这四个debug用

void print_super_block(struct super_block *sb){
	printk("start print super_block\n");
	printk("block_size %d  blocks_per_group %d  inodes_per_group %d  res_blocks %d 	blocks_count %d  inodes_count %d free_blocks_count %d  free_inodes_count %d",
		        sb->block_size, sb->blocks_per_group, \
		        sb->inodes_per_group, sb->res_blocks, \
			sb->blocks_count, sb->inodes_count,\
			sb->free_blocks_count, sb->free_inodes_count);

	printk("end printk super_block\n");
}
///注意这里打印的是group不是group_info
void print_group(struct group *gp, int cnt){
	int i = 0;
	printk("start print group\n");
	printk("b_bmp  i_bmp  i_tab  f_blk  f_ino\n");
	while(i < cnt){
		printk("%d  %d  %d  %d  %d\n", 
			gp->block_bitmap, gp->inode_bitmap, gp->inode_table,\
			gp->free_blocks_count, gp->free_inodes_count);
		++i;
		++gp;
	}
	printk("end printk group\n");
}

void print_group_info(struct group_info *gp, int cnt){
	int i = 0;
	printk("start print group\n");
	printk("b_bmp  i_bmp  i_tab  f_blk  f_ino  i_bit  b_bit  gp_nr\n");
	while(i < cnt){
		printk("%d  %d  %d  %d  %d  %d  %d  %d\n", 
			gp->block_bitmap, gp->inode_bitmap, gp->inode_table,\
			gp->free_blocks_count, gp->free_inodes_count,\
			gp->inode_bmp.bits, gp->block_bmp.bits, gp->group_nr);
		++i;
		++gp;
	}
	printk("end printk group\n");

}

/**
 * 打印元信息
 */
void print_meta_info(){
	printk("inode size : %d\n", sizeof(struct inode));
	printk("super_block size : %d\n", sizeof(struct super_block));
	printk("group size : %d\n", sizeof(struct group));
	printk("sizeof block : %d\n", BLOCK_SIZE);
}


//===================================================================================



/**
 * 分区格式化
 *
 * @attention 这里并没有创建根目录，第一次挂载时才创建根目录
 */
static void partition_format(struct partition *part){
	printk("partition %s start format...\n", part->name);

	//总块数
	uint32_t blocks = part->sec_cnt / BLK_PER_SEC;
	//组数
	uint32_t groups_cnt = blocks / GROUP_BLKS;
	//剩余块数
	uint32_t res_blks = blocks % GROUP_BLKS; 
	//组描述符所占块数
	uint32_t gp_blks = DIV_ROUND_UP(groups_cnt * sizeof(struct group), BLOCK_SIZE);

	//每组已使用的块: 超级块，块组描述符，块位图，inode位图，inode表
	uint32_t gp_used_blks = SUPER_BLKS + gp_blks + BLOCKS_BMP_BLKS \
			     + INODES_BMP_BLKS + INODES_BLKS;

//初始化超级块
	struct super_block *sb = (struct super_block *)kmalloc(SUPER_BLKS * BLOCK_SIZE);
	MEMORY_OK(sb);
	memset(sb, 0, SUPER_BLKS * BLOCK_SIZE);
 	
	sb->magic = SUPER_MAGIC;
	sb->major_rev = MAJOR;
	sb->minor_rev = MINOR;

	sb->block_size = BLOCK_SIZE;
	sb->blocks_per_group = GROUP_BLKS;
	sb->inodes_per_group = INODES_PER_GROUP;
	sb->res_blocks = res_blks;

	sb->blocks_count = blocks;
	sb->inodes_count = INODES_PER_GROUP * groups_cnt;
	//1块引导块及1块根目录块
	sb->free_blocks_count = sb->blocks_count - LEADER_BLKS - gp_used_blks * groups_cnt;
	//0号inode 给根节点
	sb->free_inodes_count = sb->inodes_count;

	//从2开始
	sb->groups_table = 2;

//初始化块组
	struct group *gp_head = (struct group *)kmalloc(gp_blks * BLOCK_SIZE);
	MEMORY_OK(gp_head);
	memset(gp_head, 0, gp_blks * BLOCK_SIZE);
	struct group *gp = gp_head;
	uint32_t cnt = 0;
	while(cnt < groups_cnt){
		gp->free_blocks_count = sb->blocks_per_group - gp_blks - SUPER_BLKS;
		gp->free_inodes_count = sb->inodes_per_group;
		gp->block_bitmap = GROUP_INNER(gp, BLOCKS_BMP_BLKS, cnt);
		gp->inode_bitmap = GROUP_INNER(gp, INODES_BMP_BLKS, cnt);
		gp->inode_table  = GROUP_INNER(gp, INODES_BLKS, cnt);
		++gp;
		++cnt;
	}
	print_group(gp_head, groups_cnt);

//块组和超级块同步磁盘
	gp = gp_head;
	cnt = groups_cnt;
	while(cnt--){
		write_direct(part, SUPER_BLK(sb, cnt), sb, SUPER_BLKS);
		write_direct(part, GROUP_BLK(sb, cnt), gp_head, gp_blks); 
	}

//创建位图
	struct bitmap bitmap;
	bitmap.byte_len = GROUP_BLKS / 8;
	bitmap.bits = (uint8_t *)kmalloc(bitmap.byte_len);
	bitmap_set_range(&bitmap, 0, 1, gp_used_blks);

//同步位图
	gp = gp_head;
	cnt = groups_cnt;
	while(cnt--){
		//写入各个组对应位图
	       	write_direct(part, gp->block_bitmap, bitmap.bits, 1);
	       	++gp;
	}

	sys_free(sb);
	sys_free(gp_head);
	printk("partition format done\n");
}

/**
 * 查看根目录是否存在，这里假设cur_gp 已经初始化
 */
static bool scan_root(struct partition *part){
	return bitmap_verify(&part->cur_gp->inode_bmp, ROOT_INODE);
}

/**
 * 创建根目录并写入磁盘
 */
static void create_root(struct partition *part){
	
	printk("create root\n");
	
	uint32_t i_no = inode_bmp_alloc(part);
	ASSERT(i_no == ROOT_INODE);

	struct inode_info *m_inode = (struct inode_info *)kmalloc(sizeof(struct inode_info));
	inode_init(part, m_inode, i_no);
	struct dir_entry dir_e;
	root_dir.inode = m_inode;

	init_dir_entry(".", ROOT_INODE, FT_DIRECTORY, &dir_e);
	add_dir_entry(&root_dir, &dir_e);
	init_dir_entry("..", ROOT_INODE, FT_DIRECTORY, &dir_e);
	add_dir_entry(&root_dir, &dir_e);
	
//	print_root(m_inode);
	inode_sync(part, m_inode);
	//确认没有指针指向，释放
	sys_free(m_inode);
	printk("create root done\n");
}

/**
 * 挂载分区，即将分区加载到内存
 *
 * @attention 这里初始化了part中的后五个成员
 */
static void mount_partition(struct partition *part){

	printk("mount_partition in %s\n", part->name);
	//设置当前分区
	cur_par = part;

//先处理超级块
	part->sb = kmalloc(sizeof(struct super_block));
	MEMORY_OK(part->sb);
//	memset(part->sb, 0, sizeof(struct super_block));
	//直接读取超级块
	read_direct(part, 1, part->sb, SUPER_BLKS);

	//初始化part中的两个字段
	part->groups_cnt = part->sb->blocks_count / part->sb->blocks_per_group;
	part->groups_blks = DIV_ROUND_UP(part->groups_cnt * sizeof(struct group), \
			BLOCK_SIZE);

//处理块组，由于块组磁盘上和内存上存储形式不同，处理方法和超级块不同

	part->cur_gp = part->groups = kmalloc(part->groups_cnt * sizeof(struct group_info));
	MEMORY_OK(part->groups);
	memset(part->groups, 0, part->groups_cnt * sizeof(struct group_info));

	//读取磁盘上块组
	uint8_t *buf = kmalloc(part->groups_blks * BLOCK_SIZE);
	struct group *gp = (struct group *)buf;
	struct group_info *gp_info = part->groups;
	read_direct(part, part->sb->groups_table, gp, part->groups_blks);

	//复制内存
	int cnt = 0;
	while(cnt < part->groups_cnt){

		memcpy(gp_info, gp, sizeof(struct group));
		gp_info->group_nr = cnt;
		++gp_info; ++gp; ++cnt;
	}
//end
	
//	print_group_info(part->groups, part->groups_cnt);

	//初始化分区缓冲
	disk_buffer_init(&part->io_buffer, part);

	//初始化第一个组
	group_info_init(part, part->cur_gp);

	//释放缓冲
	sys_free(buf);
	printk("mount partition done\n");
}

/**
 * 同步磁盘
 */
void sync(){
	struct partition *part = cur_par;

	uint8_t *buf = kmalloc(part->groups_blks * BLOCK_SIZE);
	MEMORY_OK(buf);
	struct group *gp = (struct group *)buf;
	struct group_info *gp_info = part->groups;
	struct super_block *sb = part->sb;

	uint32_t cnt = part->groups_cnt;
//复制块组
	while(cnt--){
		memcpy(gp, gp_info, sizeof(struct group));
		++gp; ++gp_info;
	}

//每个分区都要写入
	cnt = part->groups_cnt;
	while(cnt--){
		//超级块直接写入就行
		write_direct(part, SUPER_BLK(sb, cnt), sb, SUPER_BLKS);
		write_direct(part, GROUP_BLK(sb, cnt), buf, part->groups_blks);
	}

//同步块组位图
	group_bmp_sync(part);

	//根目录不在缓冲中
	inode_sync(part, root_dir.inode);
	sys_free(buf);

//同步缓冲区，这个应该放在最后
	buffer_sync(&part->io_buffer);
}

/**
 * @brief初始化文件系统
 * @detail 检查所有分区文件系统魔数，如果没有，则格式化分区，创建文件系统
 * 之后，选择默认分区挂载，打开根目录
 */
void filesys_init(){

	printk("filesys_init start\n");
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
					read_direct(part, 1, &sb, SUPER_BLKS);
					//验证魔数
					if(sb.magic == SUPER_MAGIC){
						/** nothing */
						printk("super_block has already exist\n");
						//debug 用
						//partition_format(part);
					}else {
						partition_format(part);
					}
					//如果是默认分区，则挂载
					if(!strcmp(part->name, default_part)){

					        mount_partition(part);
						//挂载分区后检测根目录
						if(!scan_root(part)){
							create_root(part);
						}
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

//初始化文件表
	uint32_t fd_idx = 0;
	while(fd_idx < MAX_FILE_OPEN){
		file_table[fd_idx++].fd_inode = NULL;
	}

	printk("filesys_init done\n");
}

/**
 * 获取文件状态
 * @param path 文件路径
 * @param st 输出缓冲
 */
int32_t sys_stat(const char *path, struct stat *st){

	struct dir_entry dir_e;
	struct dir *par_dir = search_dir_entry(cur_par, path, &dir_e);
	if(!par_dir){
		printk("sys_stat %s not found\n", path);
		return -1;
	}

	struct inode_info *m_inode = inode_open(cur_par, dir_e.i_no);
	if(!m_inode){
		printk("sys_stat open inode failed\n");
		return -1;
	}

	st->st_ino = m_inode->i_no;
	st->st_size = m_inode->i_size;
	st->st_ftype = dir_e.f_type;

	dir_close(par_dir);
	inode_close(m_inode);
	return 0;
}

//TODO debug
/**
 * 将当前目录绝对路径写入buf，size是buf大小
 * @return 失败时为NULL
 */
char *sys_getcwd(char *buf, uint32_t size){

	ASSERT(buf != NULL);
	
	uint32_t i_no = curr->cwd_inr;
	struct dir *dir = dir_open(cur_par, i_no);
	if(!dir){
		printk("sys_getcwd open dir failed\n");
		return NULL;
	}
	buf[0] = '/';
	buf[1] = '\0';

	uint32_t par_ino = 0;
	struct dir_entry dir_e;
	//根目录为0
	while(i_no){

		//找到父目录
		ASSERT(_search_dir_entry(cur_par, dir, "..", &dir_e));
		ASSERT(dir_e.f_type == FT_DIRECTORY);
		//记录inode号
		par_ino = dir_e.i_no;
		dir_close(dir);
		//打开父目录
		dir = dir_open(cur_par, par_ino); 
		ASSERT(search_dir_by_ino(cur_par, dir, i_no, &dir_e));
		ASSERT(dir_e.f_type == FT_DIRECTORY);

		strcat(buf, dir_e.filename);
		strcat(buf, "/");
		i_no = par_ino;
	}

	//倒过来
	uint32_t len = strlen(buf);
	char *head = buf;
	char *tail = buf + len - 1;
	uint32_t half = len / 2;
	while(half--)
		*head++ = *tail--;

	//去掉最后的 '/'
	buf[len - 1] = '\0';
	return buf;
}

