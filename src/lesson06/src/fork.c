#include "mm.h"
#include "sched.h"
#include "printf.h"
#include "fork.h"
#include "utils.h"

int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg)
{
	preempt_disable();
	struct task_struct *p;

	unsigned long page = allocate_kernel_page();
	p = (struct task_struct *) page;
	struct pt_regs *childregs = task_pt_regs(p);

	if (!p)
		return -1;

	if (clone_flags & PF_KTHREAD) {
		p->cpu_context.x19 = fn;
		p->cpu_context.x20 = arg;
	} else {
		struct pt_regs * cur_regs = task_pt_regs(current);
		*cur_regs = *childregs;
		childregs->regs[0] = 0;
		copy_virt_memory(p);

		unsigned long adr = PAGE_SIZE;
		unsigned long *ptr = (unsigned long *)adr;
		*ptr = 123;
		unsigned long val = *ptr;
		printf("val 1: %d\n\r", val);

		printf("pgd2: %x\n\r", p->mm.pgd);
		set_pgd(p->mm.pgd);
		*ptr = 456;
		val = *ptr;
		printf("val 2: %d\n\r", val);

		printf("pgd2: %x\n\r", task[1]->mm.pgd);
		set_pgd(task[1]->mm.pgd);
		//set_pgd(PAGE_SIZE);
		//unsigned long res = get_pgd();
		//printf("res 3: %x\n\r", res);
		val = *ptr;
		printf("val 3: %d\n\r", val);
	}
	p->flags = clone_flags;
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1; //disable preemtion untill schedule_tail

	p->cpu_context.pc = (unsigned long)ret_from_fork;
	p->cpu_context.sp = (unsigned long)childregs;
	int pid = nr_tasks++;
	task[pid] = p;	

	preempt_enable();
	return pid;
}


int move_to_user_mode(unsigned long start, unsigned long size, unsigned long pc)
{
	struct pt_regs *regs = task_pt_regs(current);
	regs->pc = pc;
	regs->pstate = PSR_MODE_EL0t;
	regs->sp = 2 *  PAGE_SIZE;  
	unsigned long code_page = allocate_user_page(current, 0);
	if (code_page == 0)	{
		return -1;
	}
	memcpy(start, code_page, size);
	set_pgd(current->mm.pgd);
	return 0;
}

struct pt_regs * task_pt_regs(struct task_struct *tsk){
	unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
	return (struct pt_regs *)p;
}
