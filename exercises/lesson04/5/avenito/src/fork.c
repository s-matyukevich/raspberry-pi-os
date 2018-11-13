#include "mm.h"
#include "sched.h"
#include "entry.h"
#include "printf.h"

int copy_process(unsigned long fn, unsigned long arg)
{
#ifdef DEBUG
	printf("Copy proccess ...\r\n"); //###
#endif
	preempt_disable();
	struct task_struct *p;

	p = (struct task_struct *) get_free_page();

#ifdef DEBUG
	printf("Free page: 0x%08x\r\n", p); //###
#endif
		
	if (!p)
		return 1;
	
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	
	p->counter = p->priority;
	
	p->preempt_count = 1; //disable preemtion until schedule_tail

	p->cpu_context.x19 = fn;
	p->cpu_context.x20 = arg;
	p->cpu_context.pc = (unsigned long)ret_from_fork;
	p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
	int pid = nr_tasks++;
	task[pid] = p;	
	
#ifdef DEBUG
	printf("task[%d] = 0x%08x\r\n", pid, p); //###
	printf("p->priority = current->priority = %d\n\r", current->priority); //###
	printf("p->state = TASK_RUNNING = %d\r\n", p->state); //###
	printf("p->counter = p->priority = %d\r\n", p->counter); //###;
	printf("p->cpu_context.x19 = 0x%08x\r\n", p->cpu_context.x19); // ###
	printf("p->cpu_context.x20 = 0x%08x\r\n", p->cpu_context.x20); // ###
	printf("p->cpu_context.pc = (unsigned long)ret_from_fork = 0x%08x\r\n", p->cpu_context.pc); // ###
	printf("p->cpu_context.sp = (unsigned long)p + THREAD_SIZE = 0x%08x\r\n", p->cpu_context.sp); // ###
#endif
	
	preempt_enable();
	return 0;
}
