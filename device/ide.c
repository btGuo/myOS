#include "ide.h"
#include "global.h"
#include "print.h"
#include "debug.h"
#include "io.h"
#include "interrupt.h"
#include "syscall.h"
#include "string.h"

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

/* 定义可读写的最大扇区数,调试用的 */
#define max_lba ((80*1024*1024/512) - 1)	// 只支持80MB硬盘


uint8_t channel_cnt;
struct ide_channel channels[2];


/*
 * bootable; 		是否可引导
 * start_head 		起始磁头号
 * start_sec; 		起始扇区号
 * start_chs; 		起始柱面号
 * fs_type;  		分区类型
 * end_head; 		结束磁头号
 * end_sec;  		结束扇区号
 * end_chs;  		结束柱面号
 * start_lba 		起始扇区lba地址
 * sec_cnt; 		本分区u扇区数量
 */
struct partition_table_entry{
	uint8_t bootable;         
	uint8_t start_head;
	uint8_t start_sec;
	uint8_t start_chs;
	uint8_t fs_type;
	uint8_t end_head;
	uint8_t end_sec;
	uint8_t end_chs;
	uint32_t start_lba;
	uint32_t sec_cnt;
}__attribute__((packed));


struct boot_sector{
	struct partition_table_entry partition_table[4];
	uint16_t signature;
}__attribute__((packed));


static void select_disk(struct disk *hd){
	uint8_t reg = BIT_DEV_MBS | BIT_DEV_LBA;
	if(hd->dev_no == 1){
		reg |= BIT_DEV_DEV;
	}
	outb(reg_dev(hd->channel), reg);
}

static void select_sector(struct disk *hd, uint32_t lba, uint8_t cnt){
	ASSERT(lba < max_lba);

	struct ide_channel *channel = hd->channel;

	outb(reg_sect_cnt(channel), cnt);
	outb(reg_lba_l(channel), lba);
	outb(reg_lba_m(channel), (lba >> 8));
	outb(reg_lba_h(channel), (lba >> 16));

	//重新写入dev 寄存器
	outb(reg_dev(channel), BIT_DEV_LBA | BIT_DEV_MBS |
			(hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

static void cmd_out(struct ide_channel *channel, uint8_t cmd){

	channel ->expecting_intr = true;
	outb(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk *hd, void *buf, uint8_t cnt){

	//以字为单位
	int size = cnt * 512 / 2;
	insw(reg_data(hd->channel), buf, size);

}

static void write2sector(struct disk *hd, void *buf, uint8_t cnt){

	int size = cnt * 512 / 2;
	outsw(reg_data(hd->channel), buf, size);

}

static void busy_wait(struct disk *hd){

	//等待
	while((inb(reg_status(hd->channel)) & BIT_STAT_BSY));
}

void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt){

	ASSERT(lba < max_lba);
	ASSERT(cnt > 0);

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

void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t cnt){
	
	ASSERT(lba < max_lba);
	ASSERT(cnt > 0);

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

static void identify_disk(struct disk *hd){

	char info[512];
	select_disk(hd);
	cmd_out(hd->channel, CMD_IDENTIFY);

	busy_wait(hd);

	read_from_sector(hd, info, 1);
	
	char buf[3][64];
	swap_copy(info + 10 * 2, buf[0], 20);
	swap_copy(info + 27 * 2, buf[1], 40);
	
	uint32_t sectors = *(uint32_t *)&info[60 * 2];
	printk("   disk %s info:\n    SN:  %s\n", hd->name, buf[0]);
	printk("      MODULE: %s\n",buf[1]);
	printk("      SECTORS  %d\n", sectors);
	printk("      CAPACITY %dMB\n", sectors * 512 / 1024 / 1024);
}

static void partition_info(struct partition *part){

	printk("   %s start_lba:%h, sec_cnt:%h\n", part->name, part->start_lba,\
			part->sec_cnt);
}
			
static void get_boot_sector(struct disk *hd, uint32_t lba, \
		struct boot_sector *bs){
	char sec[512];
	ide_read(hd, lba, sec, 1);
	//复制后66字节
	memcpy(bs, sec + 446, 66);
}

uint32_t ext_lba_base = 0;

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
		sprintf(part->name, "%s%d", hd->name, l_no + 5);
		partition_info(part);
	}
	//还有分区
	if(p[1].fs_type != 0)
		get_ext_partition(hd, l_no + 1, ext_lba_base + p[1].start_lba);
}

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
			sprintf(part->name, "%s%d", hd->name, p_no);
			partition_info(part);
		}
		++p;
	}
}

void ide_init(){

	printk("ide_init start\n");
	uint8_t hd_cnt = *((uint8_t *)(0x475));
	ASSERT(hd_cnt > 0);

	channel_cnt =  DIV_ROUND_UP(hd_cnt, 2);
	ASSERT(channel_cnt <= 2);
	uint8_t channel_no = 0;
	struct disk *hd = NULL;
	//分别处理两个通道
	while(channel_no < channel_cnt){

		sprintf(channels[channel_no].name, "ide%d", channel_no);
		
		if(channel_no == 0){
			channels[0].port_base = 0x1f0;
			channels[0].irq_no = 0x20 + 14;
		}else {
			channels[1].port_base = 0x170;
			channels[1].irq_no = 0x20 + 15;
		}
		channels[channel_no].expecting_intr = false;
		mutex_lock_init(&channels[channel_no].lock);
		sema_init(&channels[channel_no].disk_done, 0);

		//注册中断处理程序
		register_handler(channels[channel_no].irq_no, intr_hd_handler);
		//处理每个硬盘
		int dev_no = 0;
		while(dev_no < 2){
			hd = &channels[channel_no].devices[dev_no];
			hd->channel = &channels[channel_no];
			hd->dev_no = dev_no;
			sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
			//主盘为系统盘，不做处理
			identify_disk(hd);
			if(dev_no){
				partition_scan(hd, 0);
			}
			++dev_no;
		}
		++channel_no;
	}
	printk("ide_init done\n");
}

