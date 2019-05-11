#include"timer.h"
#include"io.h"
#include"print.h"
#include"thread.h"
#include"debug.h"
#include"interrupt.h"
#include"global.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT 0x40
#define PIT_CONTROL_PORT 0x43

static void intr_timer_handler(void);
extern struct task_struct *curr_thread;

uint32_t jiffies;
static void frequency_set(uint8_t counter_port, \
						  uint8_t counter_byte, \
						  uint16_t counter_value){
	outb(PIT_CONTROL_PORT, counter_byte);
	outb(counter_port, (uint8_t) counter_value);
	outb(counter_port, (uint8_t)(counter_value >> 8));
}

void timer_init(){
	put_str("timer_init start\n");
	frequency_set(COUNTER0_PORT, 0x34, COUNTER0_VALUE);
	register_handler(0x20, intr_timer_handler);
	put_str("timer_init done\n");
}

static void intr_timer_handler(void){
	ASSERT(curr_thread->stack_magic == STACK_MAGIC);
	++curr_thread->elapsed_ticks;
	++jiffies;
	if(curr_thread->ticks == 0){
		schedule();
	}else{
		--curr_thread->ticks;
	}
}
