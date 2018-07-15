#include "sched.h"
#include "irq.h"
#include "printf.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
struct task_struct *kernel_task = &(init_task);
int nr_tasks = 1;

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
	struct task_struct *p, *next;
	while (1) {
		c = -1;
		for (p = kernel_task; p; p = p->next){
			if (p->state == TASK_RUNNING && p->counter > c) {
				c = p->counter;
				next = p;
			}
		}
		if (c) {
			break;
		}
		for (p = kernel_task; p; p = p->next) {
			p->counter = (p->counter >> 1) + p->priority;
		}
	}
	switch_to(next);
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
