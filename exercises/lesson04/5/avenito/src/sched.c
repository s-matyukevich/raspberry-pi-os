#include "sched.h"
#include "irq.h"
#include "printf.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
struct task_struct * task[NR_TASKS] = {&(init_task), };
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
	printf("\n _shedule chamado\r\n"); //###
	printf("NR_TASKS: %d\r\n", NR_TASKS); //###
		
	preempt_disable();
	int next,c;
	struct task_struct * p;
	while (1) {
		c = -1;
		next = 0;
		for (int i = 0; i < NR_TASKS; i++){
			p = task[i];
			//printf("\ntask[%d]=%d\n\r", i, task[i]); //###
			//printf("state=%d\n\r", p->state); //###
			//printf("counter=%d\n\r", p->counter); //###
			if (p && p->state == TASK_RUNNING && p->counter > c) {
				c = p->counter;
				next = i;
			}
		}
		if (c) {
			break;
		}
		for (int i = 0; i < NR_TASKS; i++) {
			p = task[i];
			if (p) {
				p->counter = (p->counter >> 1) + p->priority;
			}
		}
	}
	printf("switch_to(task[%d])\n\r;", next);
	switch_to(task[next]);
	preempt_enable();
}

void schedule(void)
{
	printf("shedule chamado\r\n"); //###
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
