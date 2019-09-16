#include "fs.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "bitmap.h"
#include "string.h"
#include "group.h"
#include "buffer.h"
#include "block.h"
#include "inode.h"

#include "local.h"
#include "stat.h"

#define MAX_FS 64
static const char *root_device = "/dev/sdb1";     ///< 根分区
struct fext_fs *root_fs = NULL;                   ///< 根分区文件系统
static struct fext_fs *fs_table[MAX_FS] = {NULL};   ///< 挂载文件系统表

/**
 * 打印元信息
 */
void print_meta_info(){
	printk("inode size : %ld\n", sizeof(struct fext_inode));
	printk("super_block size : %ld\n", sizeof(struct fext_super_block));
	printk("group size : %ld\n", sizeof(struct fext_group));
}

void print_super_block(struct fext_super_block *sb){
	printk("\n");
	printk("super block information:\n");
	printk("block_size %d\t"
	        "blocks_per_group %d\t"  
		"inodes_per_group %d\t"  
		"res_blocks %d\n"
		"blocks_count %d\t"
	      	"inodes_count %d\t"
	       	"free_blocks_count %d\t"
	      	"free_inodes_count %d\n",
	        sb->block_size, 
		sb->blocks_per_group, 
	        sb->inodes_per_group, 
		sb->res_blocks,
		sb->blocks_count, 
		sb->inodes_count,
		sb->free_blocks_count, 
		sb->free_inodes_count
	);

	printk("end printk super_block\n");
}

///注意这里打印的是group不是fext_group_m
void print_group(struct fext_group *gp, int cnt){
	int i = 0;
	printk("\n");
	printk("group information:\n");
	printk("b_bmp\ti_bmp\ti_tab\tf_blk\tf_ino\n");
	while(i < cnt){
		printk("%d\t%d\t%d\t%d\t%d\n", 
			gp->block_bitmap, 
			gp->inode_bitmap, 
			gp->inode_table,
			gp->free_blocks_count, 
			gp->free_inodes_count
		);
		++i;
		++gp;
	}
}

void print_blockarg(struct fext_fs *fs){

	printk("\n");
	printk("block information:\n");
	printk("direct_blknr %u\t"
		"s_indirect_blknr %u\t"
		"d_indirect_blknr %u\t"
		"t_indirect_blknr %u\n"
		"max_blocks %u\t"
		"lba_per_blk %u\t"
		"sec_per_blk %u\t"
		"order %u\n",
		fs->direct_blknr,
		fs->s_indirect_blknr,
		fs->d_indirect_blknr,
		fs->t_indirect_blknr,
		fs->max_blocks,
		fs->lba_per_blk,
		fs->sec_per_blk,
		fs->order
	);
}

static inline void fext_set_blockarg(struct fext_fs *fs)
{
	uint32_t order = 0;
	uint32_t block_size = fs->sb->block_size;
	uint32_t size = block_size;
	while((size >>= 1)){
		order++;
	}
	order -= 2;
	fs->order = order;

	uint32_t lba_per_blk = block_size / 4;

	fs->direct_blknr = 5;
	fs->s_indirect_blknr = lba_per_blk + fs->direct_blknr;
	fs->d_indirect_blknr = fs->s_indirect_blknr + lba_per_blk * lba_per_blk;
	fs->t_indirect_blknr = fs->d_indirect_blknr + lba_per_blk * lba_per_blk * lba_per_blk;
	fs->max_blocks = fs->t_indirect_blknr;
	fs->lba_per_blk = lba_per_blk;
	fs->sec_per_blk = block_size / SECTOR_SIZE;
}
//===================================================================================

/**
 * 新建文件系统
 * @return 
 * 	@retval NULL 失败
 */
static struct fext_fs *new_fext(){
	uint32_t i = 0;
	for(; i < MAX_FS; i++){
		if(fs_table[i] == NULL){
			struct fext_fs *p = kmalloc(sizeof(struct fext_fs));
			memset(p, 0, sizeof(struct fext_fs));
			fs_table[i] = p;
			return p;
		}
	}
	return NULL;
}

/**
 * 删除文件系统，卸载时用
 */
static void delete_fext(struct fext_fs *fs){
	uint32_t i = 0;
	for(; i < MAX_FS; i++){
		if(fs_table[i] == fs){
			fs_table[i] = NULL;
			free(fs);
			return;
		}
	}
	/** do nothing */
}

/**
 * 根据分区获取该分区上的文件系统，目前只支持fext
 */
struct fext_fs *get_fext_fs(struct partition *part){
	int i = 0;
	for(;i < MAX_FS; i++){
		if(fs_table[i] && 
			fs_table[i]->sb->magic == EXT2_SUPER_MAGIC&&
			fs_table[i]->part == part){

			return fs_table[i];
		}
	}
	return NULL;
}

/**
 * 同步磁盘
 */
void sync_fext(struct fext_fs *fs){

	uint8_t *buf = kmalloc(fs->groups_blks * fs->sb->block_size);
	MEMORY_OK(buf);
	struct fext_group *gp = (struct fext_group *)buf;
	struct fext_group_m *gp_info = fs->groups;
	struct fext_super_block *sb = fs->sb;

	uint32_t cnt = fs->groups_cnt;
//复制块组
	while(cnt--){
		memcpy(gp, gp_info, sizeof(struct fext_group));
		++gp; ++gp_info;
	}

//每个分区都要写入
	cnt = fs->groups_cnt;
	while(cnt--){
		//超级块直接写入就行
		write_direct(fs, SUPER_BLK(sb, cnt), sb, SUPER_BLKS);
		write_direct(fs, GROUP_BLK(sb, cnt), buf, fs->groups_blks);
	}

//同步块组位图
	group_bmp_sync(fs);

	//根目录不在缓冲中
	inode_sync(root_fs->root_i);
	free(buf);

//同步缓冲区，这个应该放在最后
	buffer_sync(&fs->io_buffer);
}

/**
 * 查看根目录是否存在，这里假设cur_gp 已经初始化
 */
static bool scan_root(struct fext_fs *fs){
	return bitmap_verify(&fs->cur_gp->inode_bmp, ROOT_INODE);
}

/**
 * 创建根目录并写入磁盘
 */
static struct fext_inode_m *create_root(struct fext_fs *fs){
	
	printk("create root\n");
	
	uint32_t i_no = inode_bmp_alloc(fs);
	ASSERT(i_no == 0);

	struct fext_inode_m *m_inode = 
		(struct fext_inode_m *)kmalloc(sizeof(struct fext_inode_m));
	inode_init(fs, m_inode, i_no);
	struct fext_dirent dir_e;

	init_dir_entry(".", ROOT_INODE, S_IFDIR, &dir_e);
	add_dir_entry(m_inode, &dir_e);
	init_dir_entry("..", ROOT_INODE, S_IFDIR, &dir_e);
	add_dir_entry(m_inode, &dir_e);
	
//	print_root(m_inode);
	inode_sync(m_inode);
	//确认没有指针指向，释放
	printk("create root done\n");
	return m_inode;
}

/**
 * 新建fext文件系统
 * @param part 所在分区
 */
static struct fext_fs *create_fextfs(struct partition *part, bool verbose){

	if(verbose)
		printk("mount_partition in %s\n", part->name);

	struct fext_fs *fs = new_fext();
	if(fs == NULL){
		return NULL;
	}

	fs->part = part;
//先处理超级块
	
	fs->sb = kmalloc(sizeof(struct fext_super_block));
	MEMORY_OK(fs->sb);

	//直接读取超级块
	ide_read(part->disk, 
			part->start_lba + BOOT_SECS, 
			fs->sb, 
			sizeof(struct fext_super_block) / SECTOR_SIZE
		);

	if(fs->sb->magic != EXT2_SUPER_MAGIC){

		printk("super block error\n");
		return NULL;
	}
	//这个要放前面
	fext_set_blockarg(fs);

	if(verbose)
		print_super_block(fs->sb);
	if(verbose)
		print_blockarg(fs);

	uint32_t block_size = fs->sb->block_size;
	fs->groups_cnt = fs->sb->blocks_count / fs->sb->blocks_per_group;
	fs->groups_blks = DIV_ROUND_UP(fs->groups_cnt * sizeof(struct fext_group), 
			block_size);

//处理块组，由于块组磁盘上和内存上存储形式不同，处理方法和超级块不同
	fs->cur_gp = fs->groups = kmalloc(fs->groups_cnt * sizeof(struct fext_group_m));
	MEMORY_OK(fs->groups);
	memset(fs->groups, 0, fs->groups_cnt * sizeof(struct fext_group_m));

	//读取磁盘上块组
	uint8_t *buf = kmalloc(fs->groups_blks * block_size);
	struct fext_group *gp = (struct fext_group *)buf;
	struct fext_group_m *gp_info = fs->groups;

	read_direct(fs, fs->sb->groups_table, gp, fs->groups_blks);

	if(verbose)
		print_group(gp, fs->groups_cnt);

	//复制内存
	int cnt = 0;
	while(cnt < fs->groups_cnt){

		memcpy(gp_info, gp, sizeof(struct fext_group));
		gp_info->group_nr = cnt;
		++gp_info; ++gp; ++cnt;
	}
	
	//初始化分区缓冲
	disk_buffer_init(&fs->io_buffer, fs);

	//初始化第一个组
	group_info_init(fs, fs->cur_gp);

	//释放缓冲
	free(buf);

	if(verbose)
		printk("mount filesystem done\n");
	return fs;
}

void init_fs(struct partition *part, bool verbose){
	root_fs = create_fextfs(part, verbose);

	if(root_fs == NULL){
		printf("panic create fextfs error\n");
		return;
	}

	strcpy(root_fs->mount_path, "/");
	root_fs->mounted = true;

	if(!scan_root(root_fs)){

		root_fs->root_i = create_root(root_fs);
	}else {
		printf("the root inode exist!\n");
		root_fs->root_i = inode_open(root_fs, ROOT_INODE);
		printf("i_size %d\n", root_fs->root_i->i_size);
		printf("i_block[0] %d\n", root_fs->root_i->i_block[0]);
		printf("I_no %d\n", root_fs->root_i->i_no);
	}
	if(verbose)
		printk("init filesystem done\n");
}

void sync_fs(){
	sync_fext(root_fs);
	delete_fext(root_fs);
}
