#include "sched.h"
#include "irq.h"
#include "printf.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
// struct task_struct * task[NR_TASKS] = {&(init_task), };	// we don't need create the tasks now, instead
struct task_struct *initial_task = &(init_task);			// we create a initial task (reference task)
// int nr_tasks = 1;										// we don't need more

void preempt_disable(void)
{
	current->preempt_count++;
}

void preempt_enable(void)
{
	current->preempt_count--;
}


void _schedule(void)
{
	preempt_disable();
	int c;
	struct task_struct * p, * next_task;
	while (1) {
		c = -1;
		//next = 0;
		// Point to the first task (initial_task) and loop until p != 0
		for (p = initial_task; p; p = p->next_task){
			//p = task[i];   we don't need more
			if (p && p->state == TASK_RUNNING && p->counter > c) {
				c = p->counter;
				next_task = p;			// point to next task
			}
		}
		if (c) {
			break;
		}
		// Same here
		for (p = initial_task; p; p = p->next_task) {
			//p = task[i]; we don't need more
			if (p) {
				p->counter = (p->counter >> 1) + p->priority;
			}
		}
	}
	switch_to(next_task);		// next_task is a pointer to a task
	preempt_enable();
}

void schedule(void)
{
	current->counter = 0;
	_schedule();
}

void switch_to(struct task_struct * next) 
{
	if (current == next) 
		return;
	struct task_struct * prev = current;
	current = next;
	cpu_switch_to(prev, next);
}

void schedule_tail(void) {
	preempt_enable();
}


void timer_tick()
{
	--current->counter;
	if (current->counter>0 || current->preempt_count >0) {
		return;
	}
	current->counter=0;
	enable_irq();
	_schedule();
	disable_irq();
}
