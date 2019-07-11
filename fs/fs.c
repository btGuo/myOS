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
#include "print.h"
#include "thread.h"

//TODO 检查内存块复用

struct partition *cur_par = NULL; ///< 当前活跃分区
extern uint8_t channel_cnt;
extern struct ide_channel channels[2];
extern struct dir root_dir;
extern struct file file_table[MAX_FILE_OPEN];
extern struct task_struct *curr;

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


/**
 * 路径解析，提取出/之间的名字
 *
 * @param path 路径名
 * @param res  遍历结果
 *
 * @return 遍历完当前项后path的位置
 * 	@retval NULL 没有东西可以提取了
 */
static char *path_parse(const char *path, char *res){

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
static void split_path(const char *path, char *filename, char *dirname){
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
	write_direct(part, bh->blk_nr, bh->data, 1);
}


/**
 * @brief 读取磁盘块
 * @detail 先在缓冲区中查找，命中则直接返回，否则从磁盘读取块并尝试加入
 * 缓冲区，这里并没有设置脏位
 *
 * @param blk_nr 块号
 */
struct buffer_head *read_block(struct partition *part, uint32_t blk_nr){
	struct buffer_head *bh = buffer_read_block(&part->io_buffer, blk_nr);
	if(bh){
		//printk("buffer hit\n");
		return bh;
	}
	ALLOC_BH(bh);
	bh->blk_nr = blk_nr;
	bh->lock = true;
	bh->is_buffered = true;
//	bh->dirty = true;
	//从磁盘读
	read_direct(part, blk_nr, bh->data, 1);
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


//将文件系统块号转换为磁盘扇区号

/**
 * 磁盘直接写
 */
void write_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	sta_blk_nr *= BLK_PER_SEC;
	cnt *= BLK_PER_SEC;
	//这里要加上分区起始lba
	ide_write(part->disk, part->start_lba + sta_blk_nr, data, cnt);
}	

/**
 * 磁盘直接读
 */
void read_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	sta_blk_nr *= BLK_PER_SEC;
	cnt *= BLK_PER_SEC;
	//这里要加上分区起始lba
	ide_read(part->disk, part->start_lba + sta_blk_nr, data, cnt);
}


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
	struct super_block *sb = (struct super_block *)sys_malloc(SUPER_BLKS * BLOCK_SIZE);
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
	struct group *gp_head = (struct group *)sys_malloc(gp_blks * BLOCK_SIZE);
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
	bitmap.bits = (uint8_t *)sys_malloc(bitmap.byte_len);
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

	struct inode_info *m_inode = (struct inode_info *)sys_malloc(sizeof(struct inode_info));
	inode_init(part, m_inode, i_no);
	struct dir_entry dir_e;
	root_dir.inode = m_inode;

	create_dir_entry(".", ROOT_INODE, FT_DIRECTORY, &dir_e);
	add_dir_entry(&root_dir, &dir_e);
	create_dir_entry("..", ROOT_INODE, FT_DIRECTORY, &dir_e);
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
	part->sb = sys_malloc(sizeof(struct super_block));
	MEMORY_OK(part->sb);
//	memset(part->sb, 0, sizeof(struct super_block));
	//直接读取超级块
	read_direct(part, 1, part->sb, SUPER_BLKS);

	//初始化part中的两个字段
	part->groups_cnt = part->sb->blocks_count / part->sb->blocks_per_group;
	part->groups_blks = DIV_ROUND_UP(part->groups_cnt * sizeof(struct group), \
			BLOCK_SIZE);

//处理块组，由于块组磁盘上和内存上存储形式不同，处理方法和超级块不同

	part->cur_gp = part->groups = sys_malloc(part->groups_cnt * sizeof(struct group_info));
	MEMORY_OK(part->groups);
	memset(part->groups, 0, part->groups_cnt * sizeof(struct group_info));

	//读取磁盘上块组
	uint8_t *buf = sys_malloc(part->groups_blks * BLOCK_SIZE);
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

	uint8_t *buf = sys_malloc(part->groups_blks * BLOCK_SIZE);
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
 * 提取路径中最后一项目录
 *
 * @param path 目录路径，形式为 /a/b/c/ 最后一个字符是/
 * @note 对于/a/b/c/ 返回c
 */
static struct dir *get_last_dir(const char *path){
	
	//根目录直接返回
	if(!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")){
		return &root_dir;
	}

	char name[MAX_FILE_NAME_LEN];
	struct dir *par_dir = &root_dir;
	struct dir_entry dir_e;
	
	while((path = path_parse(path, name))){
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

/**
 * 打开文件
 * @param path 文件路径
 * @param flags 模式，创建或者只是打开
 *
 * @return 打开的文件号
 * 	@retval -1 失败
 */
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
//	printk("dirname : %s  dirinode %d\n", dirname, par_dir->inode->i_no);
	if(!par_dir){
		printk("open directory error\n");
		return -1;
	}

	//创建或者打开文件都需要先搜索
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
		printk("open file fd %d name %s i_no %d\n", fd, dir_e.filename, dir_e.i_no);
	}
	return fd;
}	

static inline uint32_t to_global_fd(uint32_t fd){

	ASSERT(fd >= 2 && fd < MAX_FILES_OPEN_PER_PROC);
	int32_t g_fd = curr->fd_table[fd];
	ASSERT(g_fd >= 0 && g_fd < MAX_FILE_OPEN);
	return g_fd;
}

int32_t sys_close(int32_t fd){

	uint32_t g_fd = to_global_fd(fd);
	file_close(&file_table[g_fd]);
	curr->fd_table[fd] = -1;

	return g_fd;
}

#define FD_LEGAL(fd)\
do{\
	if((fd) < 0 || (fd) >= MAX_FILES_OPEN_PER_PROC){\
		printk("sys write: fd error\n");\
		return -1;\
	}\
}while(0)



int32_t sys_write(int32_t fd, const void *buf, uint32_t count){

	FD_LEGAL(fd);
	if(fd == stdout_no){
		//TODO
		console_write(buf);
		return count;
	}
	uint32_t g_fd = to_global_fd(fd);
	struct file *file = &file_table[g_fd];
	if(file->fd_flag & O_WRONLY || file->fd_flag & O_RDWR){

		return file_write(file, buf, count);
	}else {
		printk("sys_write flag error\n");
		return -1;
	}
}

int32_t sys_read(int32_t fd, void *buf, uint32_t count){

	FD_LEGAL(fd);
	uint32_t g_fd = to_global_fd(fd);
	struct file *file = &file_table[g_fd];

	return file_read(file, buf, count);
}

int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence){

	FD_LEGAL(fd);
	ASSERT(whence > 0 && whence < 4);
	struct file *file = &file_table[to_global_fd(fd)];
	uint32_t file_size = file->fd_inode->i_size;
	int32_t new_pos = 0;

	switch(whence){
		case SEEK_SET:
			new_pos = offset;
			break;
		case SEEK_CUR:
			new_pos = file->fd_pos + offset;
			break;
		case SEEK_END:
			new_pos = file_size + offset;
			break;
	}
	//TODO 修改文件大小i_size
	if(new_pos < 0 || new_pos > file_size)
		return -1;

	file->fd_pos = new_pos;
	return new_pos;
}



