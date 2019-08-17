/**
 * command ps
 */
#include "thread.h"
#include "string.h"
#include "stdio.h"
#include "list.h"

const char *
task_stat_str[] = {"RUNNING",
                   "READY",  
                   "BLOCKED",
                   "WATTING",
                   "HANGING",
                   "DIED", 
};

const char *
head[] = { "PID", "PPID", "STAT", "TICKS", "COMMAND"};
const int size = 5;

int main(int argc, char **argv){

	int i = 0;
	for(; i < 5; i++){
		printf("%-16s", head[i]);
	}
	printf("\n");

	struct list_head *head = &thread_all_list;
	struct list_head *walk = head->next;
	struct task_struct *task = NULL;
	while(head != walk){
	
		task = list_entry(struct task_struct,  all_tag, walk);
		printf("%-16d\t%-16d\t%-16s\t%-16d\t%-16s\n", 
				task->pid, 
				task->par_pid, 
				task_stat_str[task->status],
				task->ticks,
				task->name
		      );
		walk = walk->next;
	}
}
