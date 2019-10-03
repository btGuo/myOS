#include <multiboot.h>
#include <stdint.h>
#include <string.h>
#include <kernelio.h>

#define DRIVE_MASK (1 << 7)
static char *dri_mode[] = {
	"CHS", "LBA",
};

static struct multiboot_info info;

void set_info(struct multiboot_info *_info)
{
	memcpy(&info, _info, sizeof(struct multiboot_info));
}

void print_info()
{
	printk("%x\n", &info);
	printk("%x\n", info.flags);
}

uint32_t get_mem_upper()
{
	if(info.flags & 1){

		return info.mem_upper * 1024;
	}
	return 0;
}

uint32_t get_mem_lower()
{
	if(info.flags & 1){

		return info.mem_lower * 1024;
	}
	return 0;
}

void print_drive_info()
{
	if(!(info.flags & DRIVE_MASK)){
		
		printk("no drive info\n");
		return;
	}

	uint32_t len = info.drivers_length;
	struct drive_info *d;
	uint32_t ports = 0;
	while(len)
	{
		ports = (d->size - 10) / 2;
		printk("dirve_number %d\n", d->drive_number);
		printk("drive_mode %s\n", dri_mode[d->drive_mode]);
		printk("drive_cylinders %d\n", d->drive_cylinders);
		printk("drive_heads %d\n", d->drive_heads);
		printk("drive_sectors %d\n", d->drive_sectors);

		for(int i = 0; i < ports; i++){
			printk("drive_ports_%d\n", d->drive_ports[i]);
		}
		len -= d->size;
	}
}
		
