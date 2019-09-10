#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

#include "inode.h"
#include "block.h"
#include "super_block.h"
#include "fs.h"
#include "group.h"
#include "dir.h"
#include "fs.h"
#include "file.h"

//TODO 更改block.c中的宏

struct fext_fs *fs = NULL;

struct options {
	uint32_t blocksize;
	uint32_t groupcount;
	uint32_t verbose;
	uint32_t blocks_per_group;
	char *inputdir;
	char *image;
	char *progname;
};

void copy2file(struct fext_inode_m *dest, char *src, uint32_t file_sz)
{
	int fd;
	if((fd = open(src, O_RDONLY)) == -1){
		printf("open file %s error\n", src);
		return;
	}

	char *buf = malloc(file_sz);
	if(buf == NULL){
		printf("malloc failed\n");
		return;
	}

	uint32_t sz = 0;
	if((sz = read(fd, buf, file_sz)) != file_sz){
		printf("file %s read %d\n", src, sz);
		goto done;
	}
	if((sz = inode_write(dest, buf, sz)) != file_sz){
		printf("inode %d write %d\n", dest->i_no, sz);
		goto done;
	}
done:
	free(buf);
}

void insert_directory(char *inputDir, char *target_path)
{

	struct dirent *ent;
	struct stat st;
	DIR *dir;
	char path[512];
	uint32_t child = 0;
	char t_path[512];

	if((dir = opendir(inputDir)) == NULL){
		printf("failed to opendir() `%s': %s\n",
			inputDir, strerror(errno));
		exit(EXIT_FAILURE);
    	}


	while((ent = readdir(dir))){

		if(ent->d_name[0] == '0'){
			continue;
		}

		snprintf(path, sizeof(path), "%s/%s",
				inputDir, ent->d_name);

		snprintf(t_path, sizeof(t_path), "%s/%s",
				target_path, ent->d_name);

		printf("%s\n", path);
		if(stat(path, &st) != 0){
		
	    		printf("failed to stat() `%s': %s\n",
				    path, strerror(errno));

			exit(EXIT_FAILURE);
		}

		if(S_ISDIR(st.st_mode)){

			sys_mkdir(t_path);
			insert_directory(path, t_path);
		}
		if(S_ISREG(st.st_mode)){
			
			uint32_t file_sz = st.st_size;
			struct fext_inode_m *m_inode = file_create(target_path);

			if(m_inode == NULL){
				printf("create file %s failed\n", target_path);
				continue;
			}
			copy2file(m_inode, path, file_sz);
			inode_close(m_inode);
		}
	}
	closedir(dir);	
}

void create(struct options *opt, struct partition *part){
	
	if(opt->verbose){
		printf("image %s start format...\n", opt->image);
	}

	//块大小
	uint32_t blocksize = opt->blocksize;
	//组数
	uint32_t groups_cnt = opt->groupcount;
	//每组块数量
	uint32_t blocks_per_group = opt->blocks_per_group;
	//每组inode数量
	uint32_t inodes_per_group = blocks_per_group;


	//总块数
	uint32_t blocks = blocks_per_group * groups_cnt;
	//总inodes数
	uint32_t inodes = inodes_per_group * groups_cnt;


	//inode占据的块数
	uint32_t inodes_blks = inodes_per_group * sizeof(struct fext_inode) / blocksize;
	//剩余块数
	uint32_t res_blks = blocks % blocks_per_group; 
	//组描述符所占块数
	uint32_t gp_blks = DIV_ROUND_UP(groups_cnt * sizeof(struct fext_group), blocksize);

	//每组已使用的块: 超级块，块组描述符，块位图，inode位图，inode表
	uint32_t gp_used_blks = SUPER_BLKS + gp_blks + BLOCKS_BMP_BLKS \
			     + INODES_BMP_BLKS + inodes_blks;


//初始化超级块
	struct fext_super_block *sb = (struct fext_super_block *)malloc(SUPER_BLKS * blocksize);
	MEMORY_OK(sb);
	memset(sb, 0, SUPER_BLKS * blocksize);
 	
	sb->magic = EXT2_SUPER_MAGIC;
	sb->major_rev = MAJOR;
	sb->minor_rev = MINOR;

	sb->block_size = blocksize;
	sb->blocks_per_group = blocks_per_group;
	sb->inodes_per_group = inodes_per_group;
	sb->res_blocks = res_blks;

	sb->blocks_count = blocks;
	sb->inodes_count = inodes;
	//1块引导块及1块根目录块
	sb->free_blocks_count = sb->blocks_count - LEADER_BLKS - gp_used_blks * groups_cnt;
	//0号inode 给根节点
	sb->free_inodes_count = sb->inodes_count;

	//从2开始
	sb->groups_table = 2;

//初始化块组
	struct fext_group *gp_head = (struct fext_group *)malloc(gp_blks * blocksize);
	memset(gp_head, 0, gp_blks * blocksize);
	struct fext_group *gp = gp_head;
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

	if(opt->verbose){
		//print_group(gp_head, groups_cnt);
	}
	
//块组和超级块同步磁盘
	gp = gp_head;
	cnt = groups_cnt;
	while(cnt--){
		write_direct(part, SUPER_BLK(sb, cnt), sb, SUPER_BLKS);
		write_direct(part, GROUP_BLK(sb, cnt), gp_head, gp_blks); 
	}
//创建位图
	struct bitmap bitmap;
	bitmap.byte_len = gp_blks / 8;
	bitmap.bits = (uint8_t *)malloc(bitmap.byte_len);
	bitmap_set_range(&bitmap, 0, 1, gp_used_blks);

//同步位图
	gp = gp_head;
	cnt = groups_cnt;
	while(cnt--){
		//写入各个组对应位图
	       	write_direct(part, gp->block_bitmap, bitmap.bits, 1);
	       	++gp;
	}

	if(opt->verbose){
		printf("partition format done\n");
	}
}

/** -------------------------  user face ---------------------------------- */

void print_help(char *progname){
	printf("usage: %s IMAGE [OPTIONS...]\r\n"                  				 
		"Creates a new myOS FileSystem\r\n"         				 
       		"\r\n" 										
       		" -h           Show this help message.\r\n"
	        " -v           Output verbose messages.\r\n"
  	        " -d DIRECTORY Insert files from the given directory into the image\r\n"
 	        " -b SIZE      Specifies the blocksize in bytes.\r\n"
		" -g GROUPCNT  Specifies groups to create\r\n",
		progname
	);
}

int main(int argc, char **argv){

	/** 处理命令行参数，配置文件系统   */

	struct options fs_opt;
	int opt;
	static const char *short_options = "d:b:n:l:hv";
	static struct option long_options[] = {
		{"directory" , required_argument, NULL, 'd'},
		{"blocksize" , required_argument, NULL, 'b'},
		{"groupcount" , required_argument, NULL, 'g'},
		{"help"      , no_argument      , NULL, 'h'},
		{"verbose"   , no_argument      , NULL, 'v'},
	};

	fs_opt.progname = argv[0];
	if (argc < 2){
		print_help(argv[0]);
	}

	fs_opt.image = argv[1];

	while((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1){
		switch(opt){
			case 'h':
				print_help(argv[0]);
				break;
			case 'b':
				fs_opt.blocksize = atoi(optarg);
				if(fs_opt.blocksize == 1024 ||
						fs_opt.blocksize == 2048 ||
						fs_opt.blocksize == 4096){
					break;
				}else {
					printf("input the right blocksize 1024 | 2048 | 4096\n");
					return 0;
				}
			case 'g':
				fs_opt.groupcount = atoi(optarg);
				break;
			case 'v':
				fs_opt.verbose = 1;
				break;
			case 'd':
				fs_opt.inputdir = optarg;
				break;
			default:
				printf("%s: unknown option '%s'\n",
						argv[0], optarg);
		}
	}
	fs_opt.blocks_per_group = fs_opt.blocksize * 8;


	/** 确认分区 */

	
	struct disk hd;
	ide_ctor(&hd, fs_opt.image);
	ide_scan(&hd);


	printf("input the partition serial num to select one partition\n");
	int32_t id = 0;
	while(1){
		scanf("%d", &id);

		if(id < 0 || id >= 12){
			printf("input error, illegal serial num %d\n", id);
			printf("please reinput the serial num\n");
		}
		if(id < 4 && hd.prim_parts[id].sec_cnt == 0){
			printf("no such partition %d\n", id);
		}
		if(id >= 4 && hd.logic_parts[id].sec_cnt == 0){
			printf("no such partition %d\n", id);
		}
		else {
			break;
		}
	}

	/**   选择分区  */

	struct partition *part = NULL;
	id >= 4 ? (part = &hd.logic_parts[id]) : (part = &hd.prim_parts[id]);

	/**   创建文件系统并初始化    */

	create(&fs_opt, part);
	init_fs(part);


	/**   插入目录    */
	insert_directory(fs_opt.inputdir, "/");

	ide_dtor(&hd);
}
