#include <tss.h>
#include <global.h>
#include <string.h>

struct tss{
	uint32_t  backlink;
	uint32_t* esp0;
	uint32_t  ss0;
	uint32_t* esp1;
	uint32_t  ss1;
	uint32_t* esp2;
	uint32_t  ss2;
	uint32_t  cr3;
	uint32_t  (*eip)(void);
	uint32_t  eflags;
	uint32_t  eax;
	uint32_t  ecx;
	uint32_t  edx;
	uint32_t  ebx;
	uint32_t  esp;
	uint32_t  ebp;
	uint32_t  esi;
	uint32_t  edi;
	uint32_t  es;
	uint32_t  cs;
	uint32_t  ss;
	uint32_t  ds;
	uint32_t  fs;
	uint32_t  gs;
	uint32_t  ldt;
	uint32_t  io_base; /* 高16位有效 */
};
static struct tss tss;

struct gdt_desc {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t attr_low;
	uint8_t limit_high_attr_high;
	uint8_t base_high;
};

void update_tss_esp(struct task_struct *thread){
	tss.esp0 = (uint32_t *)((uint32_t)(thread) + PG_SIZE);
}

#define TSS_ATTR_HIGH 0x80
#define TSS_ATTR_LOW  0x89
static struct gdt_desc make_gdt_desc(uint32_t base, uint32_t limit, \
		uint8_t attr_low, uint8_t attr_high){
	struct gdt_desc desc;
	desc.limit_low = limit & 0x0000ffff;
	desc.base_low = base & 0x0000ffff;
	desc.base_mid = ((base & 0x00ff0000) >> 16);
	desc.attr_low = attr_low;
	desc.limit_high_attr_high = attr_high | ((limit & 0x000f0000) >> 16);
	desc.base_high = ((base & 0xff000000) >> 24);
	return desc;
}

void tss_init(){
	uint32_t gdt_base = 0;
	uint64_t gdt_operand = 0;

	asm volatile("sgdt %0"::"m"(gdt_operand));

	gdt_base = gdt_operand >> 16;
	printk("gdt base addr is %x\n", gdt_base);

	printk("tss init start\n");
	uint32_t tss_size = sizeof(tss);
	memset(&tss, 0, tss_size);
	tss.ss0 = SELECTOR_K_STACK;
	tss.io_base = (tss_size << 16);

	*((struct gdt_desc *)(gdt_base + 0x20)) = make_gdt_desc(\
			(uint32_t)&tss, tss_size - 1, 0x89, 0x80);	

	*((struct gdt_desc *)(gdt_base + 0x28)) = make_gdt_desc(\
			0, 0x000fffff, 0xf8, 0xc0);

	*((struct gdt_desc *)(gdt_base + 0x30)) = make_gdt_desc(\
			0, 0x000fffff, 0xf2, 0xc0);

	uint16_t *gdt_limit = (uint16_t *)&gdt_operand;
	*gdt_limit = 8 * 7 - 1;

	asm volatile("lgdt %0"::"m"(gdt_operand));
	asm volatile("ltr %w0"::"r"(SELECTOR_K_TSS));
	printk("tss init done\n");
}

