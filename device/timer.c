#include "timer.h"
#include "io.h"
#include "thread.h"
#include "interrupt.h"
#include "global.h"
#include "kernelio.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT 0x40
#define PIT_CONTROL_PORT 0x43

#define mil_sec_per_intr (1000 / IRQ0_FREQUENCY)

static void intr_timer_handler(void);
extern struct task_struct *curr;

uint32_t jiffies;
static void frequency_set(uint8_t counter_port, \
						  uint8_t counter_byte, \
						  uint16_t counter_value){
	outb(PIT_CONTROL_PORT, counter_byte);
	outb(counter_port, (uint8_t) counter_value);
	outb(counter_port, (uint8_t)(counter_value >> 8));
}

void timer_init(){
	printk("timer_init start\n");
	frequency_set(COUNTER0_PORT, 0x34, COUNTER0_VALUE);
	register_handler(0x20, intr_timer_handler);
	printk("timer_init done\n");
}

static void intr_timer_handler(void){
	ASSERT(curr->stack_magic == STACK_MAGIC);
	++curr->elapsed_ticks;
	++jiffies;
	if(curr->ticks == 0){
		schedule();
	}else{
		--curr->ticks;
	}
}

static void ticks_to_sleep(uint32_t sleep_ticks){
	uint32_t start_tick = jiffies;
	while(jiffies - start_tick < sleep_ticks){
		thread_yield();
	}
}

void mtime_sleep(uint32_t m_secs){
	uint32_t sleep_ticks = DIV_ROUND_UP(m_secs, mil_sec_per_intr);
	ASSERT(sleep_ticks > 0);
	ticks_to_sleep(sleep_ticks);
}
