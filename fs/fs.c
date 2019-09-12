#include <ide.h>
#include <fs.h>
#include <inode.h>
#include <super_block.h>
#include <dir.h>
#include <global.h>
#include <bitmap.h>
#include <string.h>
#include <memory.h>
#include <group.h>
#include <buffer.h>
#include <file.h>
#include <debug.h>
#include <thread.h>
#include <block.h>
#include <inode.h>
#include <ide.h>

#define FEXT_SUPER_SIZE sizeof(struct fext_super_block)
#define MAX_FS 64
static const char *root_device = "/dev/sdb1";     ///< 根分区
struct fext_fs *root_fs = NULL;                   ///< 根分区文件系统
static struct fext_fs *fs_table[MAX_FS] = {NULL};   ///< 挂载文件系统表


#define DEBUG 1
//==========================================================================
#ifdef DEBUG

void print_super_block(struct fext_super_block *sb){
	printk("start print super_block\n");
	printk("block_size %d\t"
	        "blocks_per_group %d\t"  
		"inodes_per_group %d\t"  
		"res_blocks %d\t"
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
	printk("start print group\n");
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
	printk("end printk group\n");
}

void print_fext_group_m(struct fext_group_m *gp, int cnt){
	int i = 0;
	printk("start print group\n");
	printk("b_bmp\ti_bmp\ti_tab\tf_blk\tf_ino\ti_bit\tb_bit\tgp_nr\n");
	while(i < cnt){
		printk("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", 
			gp->block_bitmap, 
			gp->inode_bitmap, 
			gp->inode_table,
			gp->free_blocks_count, 
			gp->free_inodes_count,
			gp->inode_bmp.bits, 
			gp->block_bmp.bits, 
			gp->group_nr
		);
		++i;
		++gp;
	}
	printk("end printk group\n");

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

#endif

/**
 * 打印元信息
 */
void print_meta_info(){
	printk("inode size : %d\n", sizeof(struct fext_inode));
	printk("super_block size : %d\n", sizeof(struct fext_super_block));
	printk("group size : %d\n", sizeof(struct fext_group));
	printk("sizeof block : %d\n", root_fs->sb->block_size);
}


static void fext_set_blockarg(struct fext_fs *fs)
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
	//先在全局表中删除
	uint32_t i = 0;
	for(; i < MAX_FS; i++){
		if(fs_table[i] == fs){
			fs_table[i] = NULL;
			break;
		}
	}
	//释放相关资源
	kfree(fs->sb);
	kfree(fs->groups);
	kfree(fs->root_i);
	kfree(fs);
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

	uint32_t block_size = fs->sb->block_size;
	uint8_t *buf = kmalloc(fs->groups_blks * block_size);
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
	kfree(buf);

//同步缓冲区，这个应该放在最后
	buffer_sync(&fs->io_buffer);
}

/**
 * 新建fext文件系统
 * @param part 所在分区
 */
static struct fext_fs *create_fextfs(struct partition *part){

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
		delete_fext(fs);
		return NULL;
	}

	fext_set_blockarg(fs);

	uint32_t block_size = fs->sb->block_size;
	fs->groups_cnt = fs->sb->blocks_count / fs->sb->blocks_per_group;
	fs->groups_blks = DIV_ROUND_UP(fs->groups_cnt * sizeof(struct fext_group), \
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
	kfree(buf);

	//print_super_block(fs->sb);
	//print_fext_group_m(fs->cur_gp, 1);
	printk("mount fsition done\n");
	return fs;
}

/**
 * 卸载文件系统
 */
int32_t sys_unmount(const char *device){

	struct fext_inode_m *m_inode = path2inode(device);
	if(m_inode == NULL){
		printk("sys_mount: error device name %s\n", device);
		return -1;
	}
	if(!S_ISBLK(m_inode->i_mode)){
		printk("sys_mount: error device type\n");
		return -1;
	}
	inode_close(m_inode);

	struct fext_fs *fs = get_fext_fs(name2part(device));
	if(fs == root_fs){
		return -1;
	}

	if(!fs->mounted){
		return -1;
	}
	struct fext_inode_m *mounted_i = path2inode(fs->mount_path);
	if(mounted_i == NULL){
		return -1;
	}
	mounted_i->i_mounted = false;
	//TODO 这里不太确定
	mounted_i->fs = root_fs;

	if(buffer_check_inode(&fs->io_buffer)){
		printk("sys_mount: there are some inodes being used\n");
		return -1;
	}
	//同步后释放fs
	sync_fext(fs);
	delete_fext(fs);

	return 0;
}

/**
 * 挂载目录
 * @param device 设备文件名
 * @param dir_path 挂载点目录名
 */
int32_t sys_mount(const char *device, const char *dir_path){

	//只读方式打开设备文件
	struct fext_inode_m *m_inode = path2inode(device);
	if(m_inode == NULL){
		printk("sys_mount: error device name %s\n", device);
		return -1;
	}
	if(!S_ISBLK(m_inode->i_mode)){
		printk("sys_mount: error device type\n");
		return -1;
	}
	inode_close(m_inode);

	struct partition *part = name2part(device);
	if(part == NULL){
		printk("sys_mount: no such device\n");
		return -1;
	}
	struct fext_inode_m *inode = path2inode(dir_path);
	if(inode == NULL){
		printk("sys_mount: open dir %s failed\n", dir_path);
		return -1;
	}
	if(inode->i_mounted){
		printk("sys_mount: directory %s has been used\n", dir_path);
		return -1;
	}
	struct fext_fs *fs = get_fext_fs(part);
	if(fs){
		printk("sys_mount: partition %s has been mounted\n", device);
		return -1;
	}
	fs = create_fextfs(part);

	//设置已经挂载
	fs->mounted = true;
	strcpy(fs->mount_path, dir_path);
	fs->root_i = inode_open(fs, ROOT_INODE);
	//重新设置inode号，fs
	inode->i_mounted = true;
	inode->fs = fs;
	
	//加载根目录
	return 0;
}


/**
 * @brief初始化文件系统
 * @detail 检查所有分区文件系统魔数，如果没有，则格式化分区，创建文件系统
 * 之后，选择默认分区挂载，打开根目录
 */
void filesys_init(){

#ifdef DEBUG
	printk("filesys_init start\n");
#endif
//加载根文件系统
	root_fs = create_fextfs(name2part(root_device));
	if(root_fs == NULL){
		PANIC("build root_fs failed\n");
	}
	
	struct fext_inode_m *inode = inode_open(root_fs, ROOT_INODE);
	if(inode == NULL){
		PANIC("can't find root inode\n");
	}
	inode->i_mounted = true;

	root_fs->root_i = inode;
	strcpy(root_fs->mount_path, "/");
	root_fs->mounted = true;

	//print_super_block(root_fs->sb);
	printk("i_size %d\n", inode->i_size);
	printk("i_block[0] %d\n", inode->i_block[0]);
	printk("i_block[1] %d\n", inode->i_block[1]);
	printk("i_block[2] %d\n", inode->i_block[2]);
	printk("i_block[3] %d\n", inode->i_block[3]);
	printk("i_block[4] %d\n", inode->i_block[4]);
	printk("i_block[5] %d\n", inode->i_block[5]);
	printk("i_block[6] %d\n", inode->i_block[6]);
	printk("i_block[7] %d\n", inode->i_block[7]);

//初始化文件表
	uint32_t fd_idx = 0;
	while(fd_idx < MAX_FILE_OPEN){
		file_table[fd_idx++].fd_inode = NULL;
	}

#ifdef DEBUG
	printk("filesys_init done\n");
#endif
}

//============================================================================

/**
 * 获取文件状态
 * @param path 文件路径
 * @param st 输出缓冲
 */
int32_t sys_stat(const char *path, struct stat *st){

	struct fext_dirent dir_e;
	struct fext_inode_m *par_i = search_dir_entry(path, &dir_e);
	if(!par_i){
		printk("sys_stat %s not found\n", path);
		return -1;
	}

	struct fext_inode_m *m_inode = inode_open(par_i->fs, dir_e.i_no);
	if(!m_inode){
		printk("sys_stat open inode failed\n");
		return -1;
	}

	st->st_ino = m_inode->i_no;
	st->st_size = m_inode->i_size;
	st->st_mode = dir_e.f_type;

	inode_close(par_i);
	inode_close(m_inode);
	return 0;
}

//TODO debug
/**
 * 将当前目录绝对路径写入buf，size是buf大小
 * @return 失败时为NULL
 */

char *sys_getcwd(char *buf, uint32_t size){

	char *bufhead = buf;
	ASSERT(buf != NULL);
	
	ASSERT(curr->cwd_i);
	struct fext_inode_m *inode = curr->cwd_i;
	uint32_t i_no = inode->i_no;
	struct fext_fs *fs = inode->fs;

	ASSERT(inode->fs);

	//补上挂载路径
	strcpy(buf, fs->mount_path);

	//挂载点，直接复制挂载路径后返回
	if(i_no == 0){
		return buf;
	}

	//如果不是 "/" 在后面加上 "/"
	if(strlen(buf) != 1){
		strcat(buf, "/");
		buf += strlen(buf) - 1;
	}

	struct fext_inode_m *dir_i = NULL;
	uint32_t par_ino = 0;
	struct fext_dirent dir_e;
	uint32_t cnt = 4;
	//根目录为0
	while(i_no){

		//找到父目录
		ASSERT(_search_dir_entry(inode, "..", &dir_e));
		ASSERT(S_ISDIR(dir_e.f_type));
		//记录inode号
		par_ino = dir_e.i_no;

		// 这里注意，如果是cwd则不能关闭该目录
		if(inode != curr->cwd_i){
			inode_close(inode);
		}
		//打开父目录
		dir_i = inode_open(fs, dir_e.i_no);
		ASSERT(search_dir_by_ino(dir_i, i_no, &dir_e));
		ASSERT(S_ISDIR(dir_e.f_type));

		strcat(buf, dir_e.filename);
		strcat(buf, "/");

		i_no = par_ino;
		inode = dir_i;
		if(cnt-- == 0){
			break;
		}
	}

	//printk("before swap : %s\n", buf);
	//去掉最后的 '/'
	uint32_t len = strlen(buf);
	buf[len - 1] = '\0';
	

	//倒过来
	char *swap = kmalloc(len + 1);
	strcpy(swap, buf);
	buf[0] = '\0';
	while(len--){
		if(swap[len] == '/'){
			strcat(buf, swap + len);
			swap[len] = '\0';
		}
	}

	kfree(swap);
	return bufhead;
}

/**
 * 切换当前工作目录
 */
int32_t sys_chdir(const char *path){

	struct fext_inode_m *inode = path2inode(path);
	if(inode == NULL){
		return -1;
	}
	inode_close(curr->cwd_i);
	curr->cwd_i = inode;
	return 0;
}

