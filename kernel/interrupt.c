#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"
#include "thread.h"

#define IDT_DESC_CNT 0x81
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define EFLAGS_IF 0x00000200
//无法直接访问eflags入栈后再出栈
#define GET_EFLAGS(EFLAGS_VAR) asm volatile("pushfl; popl %0" : "=g"(EFLAGS_VAR))


struct gate_desc{
	uint16_t func_offset_low_word;
	uint16_t selector;
	uint8_t  dcount;
	uint8_t  attribute;
	uint16_t func_offset_high_word;
};

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];

char *intr_name[IDT_DESC_CNT];
intr_handler idt_table[IDT_DESC_CNT];

extern intr_handler intr_entry_table[IDT_DESC_CNT];
extern uint32_t syscall_handler(void);

/**
 * 初始化8259
 */
static void pic_init(void){
	/* 初始化主片，ICW1 - ICW4 */
	outb(PIC_M_CTRL, 0x11);
	outb(PIC_M_DATA, 0x20);
	outb(PIC_M_DATA, 0x04);
	outb(PIC_M_DATA, 0x01);
	/*初始化从片，ICW1 - ICW4*/
	outb(PIC_S_CTRL, 0x11);
	outb(PIC_S_DATA, 0x28);
	outb(PIC_S_DATA, 0x02);
	outb(PIC_S_DATA, 0x01);
	/* ocw1 */
	outb(PIC_M_DATA, 0xf8);
	outb(PIC_S_DATA, 0xbf);

	printk("   pic_init done\n");
}

/**
 * 构建idt描述符
 */
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function){
	p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
}

/**
 * 初始化中断描述符表
 */
static void idt_desc_init(void){ 
	int i; 
	for(i = 0; i < IDT_DESC_CNT; ++i)
	{
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
		if(i == IDT_DESC_CNT - 1)
			make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL3, syscall_handler);
	}
	printk("   idt_desc_init done\n");
}

static void general_intr_handler(uint8_t vec_nr, uint32_t err_code){
	if(vec_nr == 0x27 || vec_nr == 0x2f){
		return;
	}
	printk("\nint vector : %d\n%s\n", vec_nr, intr_name[vec_nr]);
	while(1);
}

#define DEBUG 1

static void page_fault_handler(uint8_t vec_nr, uint32_t err_code){

	ASSERT(vec_nr == 0xe);

	uint32_t vaddr = 0;
	asm("movl %%cr2, %0":"=r"(vaddr));

#ifdef DEBUG
	printk("error code is %d\n", err_code);;
	printk("curr->pid :%d\n", curr->pid); 
	printk("page fault address is : %h\n", vaddr);
#endif

	if(err_code & 0x1){
		do_wp_page(vaddr);
	}else {
		do_page_fault(vaddr);
	}
}

static void exception_init(void){
	int i;
	for(i = 0; i < IDT_DESC_CNT; ++i){
		idt_table[i] = general_intr_handler;
		intr_name[i] = "unknown";
	}
	intr_name[0] = "#DE Divide Error";
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMT Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF overflow Exception";
	intr_name[5] = "#BR Bound Range Exceeded Exception";
	intr_name[6] = "#UD Invalid Opcode Exception";
	intr_name[7] = "#NM Device not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invalid TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";
	
	//注册缺页处理程序
	idt_table[14] = page_fault_handler;
}

enum intr_status intr_enable(){
	if(INTR_ON == intr_get_status()){
		return INTR_ON;
	}else{
		asm volatile("sti");
		return INTR_OFF;
	}
}

enum intr_status intr_disable(){
	if(intr_get_status() == INTR_ON){
		asm volatile("cli":::"memory");
		return INTR_ON;
	}else{
		return INTR_OFF;
	}
}

enum intr_status intr_set_status(enum intr_status status){
	return status & INTR_ON ? intr_enable(): intr_disable();
}

enum intr_status intr_get_status(){
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}	

void register_handler(uint8_t vec_nr, intr_handler function){
	idt_table[vec_nr] = function;
}

void idt_init(){
	printk("idt_init start\n");
	idt_desc_init();
	exception_init();
	pic_init();

	uint64_t idt_operand = ((sizeof(idt) - 1) | 
			((uint64_t)(uint32_t)idt << 16));
	asm volatile("lidt %0"::"m"(idt_operand));
	printk("idt_init done\n");
}

