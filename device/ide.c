#include "ide.h"
#include "global.h"
#include "kernelio.h"
#include "io.h"
#include "interrupt.h"
#include "string.h"
#include "tty.h"

#define reg_data(channel)	 (channel->port_base + 0)
#define reg_error(channel)	 (channel->port_base + 1)
#define reg_sect_cnt(channel)	 (channel->port_base + 2)
#define reg_lba_l(channel)	 (channel->port_base + 3)
#define reg_lba_m(channel)	 (channel->port_base + 4)
#define reg_lba_h(channel)	 (channel->port_base + 5)
#define reg_dev(channel)	 (channel->port_base + 6)
#define reg_status(channel)	 (channel->port_base + 7)
#define reg_cmd(channel)	 (reg_status(channel))
#define reg_alt_status(channel)  (channel->port_base + 0x206)
#define reg_ctl(channel)	 reg_alt_status(channel)

/* reg_status寄存器的一些关键位 */
#define BIT_STAT_BSY	 0x80	      // 硬盘忙
#define BIT_STAT_DRDY	 0x40	      // 驱动器准备好	 
#define BIT_STAT_DRQ	 0x8	      // 数据传输准备好了

/* device寄存器的一些关键位 */
#define BIT_DEV_MBS	0xa0	    // 第7位和第5位固定为1
#define BIT_DEV_LBA	0x40        // 第6位，启用lba模式
#define BIT_DEV_DEV	0x10        // 第4位，指定主盘或者从盘

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY	   0xec	    // identify指令
#define CMD_READ_SECTOR	   0x20     // 读扇区指令
#define CMD_WRITE_SECTOR   0x30	    // 写扇区指令

/* 定义可读写的最大块数 */
#define MAX_LBA (1024 * 1024 * 89)

uint8_t channel_cnt;     ///< 通道数 1或者2 
struct ide_channel channels[2];  ///< 两条ide通道
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

/**
 * @brief 引导扇区描述符mbr 或 ebr
 */
struct boot_sector{
	struct partition_table_entry partition_table[4];   ///< 四个分区表项
	uint16_t signature;   ///< 魔数
}__attribute__((packed));

/**
 * 根据设备号查找分区
 * @note 这里只是简单遍历
 */
struct partition *get_part(dev_t dev_nr){
	uint32_t i = 0;
	while(i < MAX_PARTS){
		if(parts[i]->dev_nr == dev_nr){
			return parts[i];
		}
		i++;
	}
	return NULL;
}

/**
 * 根据分区文件名获取分区
 */
struct partition *name2part(const char *device){
	const char *name = 1 + strrchr(device, '/');
	//printk("name %s\n", name);
	uint32_t i = 0;
	for(; i < MAX_PARTS && parts[i]; i++){
		if(strcmp(parts[i]->name, name) == 0){
			return parts[i];
		}
	}
	return NULL;
}

/**
 * 选择硬盘
 */
static void select_disk(struct disk *hd){
	uint8_t reg = BIT_DEV_MBS | BIT_DEV_LBA;
	if(hd->dev_no == 1){
		reg |= BIT_DEV_DEV;
	}
	outb(reg_dev(hd->channel), reg);
}

/**
 * 选择扇区
 * @param lba 扇区lba地址
 * @param cnt 要操作的扇区数
 */
static void select_sector(struct disk *hd, uint32_t lba, uint8_t cnt){
	ASSERT(lba < MAX_LBA);

	struct ide_channel *channel = hd->channel;

	outb(reg_sect_cnt(channel), cnt);
	outb(reg_lba_l(channel), lba);
	outb(reg_lba_m(channel), (lba >> 8));
	outb(reg_lba_h(channel), (lba >> 16));

	//重新写入dev 寄存器
	outb(reg_dev(channel), BIT_DEV_LBA | BIT_DEV_MBS |
			(hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

/**
 * 往通道写入命令
 */
static inline void cmd_out(struct ide_channel *channel, uint8_t cmd){

	outb(reg_cmd(channel), cmd);
}

/**
 * 读扇区
 * @param buf 输出缓冲区
 * @param cnt 读取扇区数
 */
static inline void read_from_sector(struct disk *hd, void *buf, uint8_t cnt){

	//以字为单位
	int size = cnt * 512 / 2;
	insw(reg_data(hd->channel), buf, size);

}

/**
 * 写入扇区
 * @param buf 输入缓冲区
 * @param cnt 写入扇区数
 */
static inline void write2sector(struct disk *hd, void *buf, uint8_t cnt){

	int size = cnt * 512 / 2;
	outsw(reg_data(hd->channel), buf, size);

}

/**
 * 等待硬盘就绪
 */
static inline void busy_wait(struct disk *hd){

	//等待
	while((inb(reg_status(hd->channel)) & BIT_STAT_BSY));
}

/**
 * @brief 硬盘读
 * 
 * @param hd 磁盘分区指针
 * @param lba 扇区号
 * @param cnt 扇区数
 */
void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt){

	ASSERT(lba < MAX_LBA && cnt > 0);
	//printk("ide_read at %d\n", lba);

	mutex_lock_acquire(&hd->channel->lock);

	select_disk(hd);

	uint32_t reads = 0;
	uint32_t res = cnt;
	uint32_t to_read = 0;

	while(res){
		
		if(res < 256){
			to_read = res;
			res = 0;
		}else {
			to_read = 256;
			res -= 256;
		}

		select_sector(hd, lba + reads, to_read);
		cmd_out(hd->channel, CMD_READ_SECTOR);
		
		busy_wait(hd);

		read_from_sector(hd, (void*)((uint32_t)buf + reads * 512), to_read);

		reads += to_read;
	}
	mutex_lock_release(&hd->channel->lock);
}

/**
 * 写硬盘
 */
void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt){
	
	ASSERT(lba < MAX_LBA && cnt > 0);

	mutex_lock_acquire(&hd->channel->lock);

	select_disk(hd);

	uint32_t writes = 0;
	uint32_t res = cnt;
	uint32_t to_write = 0;
	while(res){

		if(res < 256){
			to_write = res;
			res = 0;
		}else {
			to_write = 256;
			res -= 256;
		}

		select_sector(hd, lba + writes, to_write);
		cmd_out(hd->channel, CMD_WRITE_SECTOR);

		busy_wait(hd);

		write2sector(hd, (void*)((uint32_t)buf + writes * 512), to_write);

		writes += to_write;
	}

	mutex_lock_release(&hd->channel->lock);
}


void intr_hd_handler(uint8_t irq_no){
	/* empty */
}

static void swap_copy(const char *src, char *buf, uint32_t size){
	
	int i;
	for(i = 0; i < size; i += 2){
		buf[i + 1] = *src++;
		buf[i] = *src++;
	}
	buf[size] = '\0';
}

#define DEBUG 1

/**
 * 识别磁盘
 * @return 硬盘是否存在
 */
static bool identify_disk(struct disk *hd){

	char info[512];
	select_disk(hd);
	cmd_out(hd->channel, CMD_IDENTIFY);

	busy_wait(hd);

	read_from_sector(hd, info, 1);
	
	char buf[3][64];
	swap_copy(info + 10 * 2, buf[0], 20);
	swap_copy(info + 27 * 2, buf[1], 40);
	
	uint32_t sectors = *(uint32_t *)&info[60 * 2];

	if(sectors){

		hd->sectors = sectors;
#ifdef DEBUG
		printk("\tdisk %s info:\n\tSN:  %s\n", hd->name, buf[0]);
		printk("\t\tMODULE: %s\n",buf[1]);
		printk("\t\tSECTORS  %d\n", sectors);
		printk("\t\tCAPACITY %dMB\n", sectors * 512 / 1024 / 1024);
#endif
		return true;
	}
	return false;
}

/**
 * 打印分区信息
 */
static void partition_info(struct partition *part){

	printk("\t%s start_lba:%x, sec_cnt:%x\n", 
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
		part->disk = hd;
		part->dev_nr = IDE_MAJOR << MAJOR_SHIFT |
			hd->dev_no << MINOR_SHIFT | (l_no + 5);

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
			part->disk = hd;
			part->dev_nr = IDE_MAJOR << MAJOR_SHIFT |
				hd->dev_no << MINOR_SHIFT | p_no;

			parts[part_idx++] = part;
			sprintf(part->name, "%s%d", hd->name, p_no);
			partition_info(part);
		}
		++p;
	}
	printk("scan done\n");
}

/**
 * 简单测试分区读写
 */
#ifdef DEBUG
void test(struct partition *part){
	printk("test %s\n", part->name);
	char buf[512];
	memset(buf, 0, 512);
	ide_read(part->disk, part->start_lba + 3, buf, 1);
	ide_write(part->disk, part->start_lba + 3, buf, 1);
	ide_write(part->disk, part->start_lba + 3, buf, 1);
	printk("end test\n");
}
#endif

/**
 * @brief硬盘初始化
 * @detail 初始化两条通道，以及其中的硬盘及分区描述符
 * @attention 分区描述符只初始化了前四项，其余在这里均未初始化，在文件系统处被初始化
 */
void ide_init(){
#ifdef DEBUG
	printk("ide_init start\n");
#endif
	uint8_t hd_cnt = *((uint8_t *)(0x475));
	printk("total channels %d\n", hd_cnt);
	ASSERT(hd_cnt > 0);

	channel_cnt =  DIV_ROUND_UP(hd_cnt, 2);
	ASSERT(channel_cnt <= 2);
	uint8_t channel_no = 0;
	struct disk *hd = NULL;
	//分别处理两个通道
	while(channel_no < channel_cnt){

		sprintf(channels[channel_no].name, "ide%d", channel_no);
		
		printk("%s\n", channels[channel_no].name);
		
		if(channel_no == 0){
			channels[0].port_base = 0x1f0;
			channels[0].irq_no = 0x20 + 14;
		}else {
			channels[1].port_base = 0x170;
			channels[1].irq_no = 0x20 + 15;
		}
//		channels[channel_no].expecting_intr = false;
		mutex_lock_init(&channels[channel_no].lock);
	//	sema_init(&channels[channel_no].disk_done, 0);

		//注册中断处理程序
		register_handler(channels[channel_no].irq_no, intr_hd_handler);
		//处理每个硬盘
		int dev_no = 0;
		while(dev_no < 2){

			printk("ide_init %d\n", dev_no);
			hd = &channels[channel_no].devices[dev_no];
			hd->channel = &channels[channel_no];
			hd->dev_no = dev_no;
			sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);

			printk("%s\n", hd->name);
			if(identify_disk(hd)){

				partition_scan(hd, 0);
			}
			
			++dev_no;
		}
		++channel_no;
	}
#ifdef DEBUG
	printk("ide_init done\n");
#endif
}
