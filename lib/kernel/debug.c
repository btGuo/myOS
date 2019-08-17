#include "interrupt.h"
#include "debug.h"

void panic_spin(char *filename, int line, const char *func, const char *condition){
	STI();

	printk("\n========== error ===========\n");
	printk("filename : %s\n", filename);
	printk("line : %d\n", line);
	printk("function : %s\n", func);
	printk("condition : %s\n", condition);

	while(1);
}

