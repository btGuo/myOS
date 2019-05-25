#include "ide.h"
#include "global.h"
#include "print.h"
#include "debug.h"

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
#define BIT_DEV_LBA	0x40
#define BIT_DEV_DEV	0x10

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY	   0xec	    // identify指令
#define CMD_READ_SECTOR	   0x20     // 读扇区指令
#define CMD_WRITE_SECTOR   0x30	    // 写扇区指令

/* 定义可读写的最大扇区数,调试用的 */
#define max_lba ((80*1024*1024/512) - 1)	// 只支持80MB硬盘


uint8_t channel_cnt;
struct ide_channel channels[2];

void ide_init(){
	printk("ide_init_start\n");
	uint8_t hd_cnt = *((uint8_t *)(0x475));
	ASSERT(hd_cnt > 0);

	channel_cnt =  DIV_ROUND_UP(hd_cnt, 2);
	ASSERT(channel_cnt <= 2);
	uint8_t channel_no = 0;
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

		++channel_no;
	}
	printk("ide_init done\n");
}

