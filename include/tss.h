#ifndef __KERNEL_TSS_H
#define __KERNEL_TSS_H

#include "thread.h"

void update_tss_esp(struct task_struct *thread);
void tss_init();

#endif
