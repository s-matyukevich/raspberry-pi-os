#include "printf.h"
#include "mm.h"
#include "sched.h"
#include "entry.h"

int copy_process(unsigned long fn, unsigned long arg)
{
	preempt_disable();
	struct task_struct *p, *previous_task;

	p = (struct task_struct *) get_free_page();
	if (!p)
		return 1;
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1; 					//disable preemtion until schedule_tail

	p->cpu_context.x19 = fn;
	p->cpu_context.x20 = arg;
	p->cpu_context.pc = (unsigned long)ret_from_fork;
	p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
	
	p->next_task = 0;						// the just created task shall point to 0, that means it's the last one created
	
	//int pid = nr_tasks++;					// we don't need more
	//task[pid] = p;
	
	previous_task = initial_task;			// point to the first task (initial task)
	
	// Find the last created task (it points to 0)
	// run while previous_task->next_task != 0
	while(previous_task->next_task) previous_task = previous_task->next_task;
	
	previous_task->next_task = p;			// point the last one to this just created one			
	
	printf("\n\r----------- Task[%d] created -----------\r\n", pid);
	printf("\n\rStruct task allocated at 0x%08x.\r\n", p);
	printf("p->cpu_context.x19 = 0x%08x. (fn)\r\n", p->cpu_context.x19);
	printf("p->cpu_context.x20 = 0x%08x. (arg)\r\n", p->cpu_context.x20);
	printf("p->cpu_context.pc  = 0x%08x. (ret_from_fork)\r\n", p->cpu_context.pc);
	printf("p->cpu_context.sp  = 0x%08x. (sp)\r\n", p->cpu_context.sp);

	preempt_enable();
	return 0;
}
