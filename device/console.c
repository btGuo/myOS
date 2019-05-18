#include "console.h"
#include "sync.h"
#include "print.h"

static struct mutex_lock console_lock;

void console_init(){
	mutex_lock_init(&console_lock);
}

void console_write(const char *str){
	mutex_lock_acquire(&console_lock);
	put_str(str);
	mutex_lock_release(&console_lock);	
}

