#include "entry.h"
#include "mm.h"
#include "sched.h"

int copy_process(unsigned long fn, unsigned long arg, long pri) {
  preempt_disable();
  struct task_struct *p;

  p = (struct task_struct *)get_free_page();
  if (!p)
    return 1;
  p->priority = pri;
  p->state = TASK_RUNNING;
  p->counter = p->priority;
  p->preempt_count = 1; // disable preemtion untill schedule_tail
  p->next = 0;

  p->cpu_context.x19 = fn;
  p->cpu_context.x20 = arg;
  p->cpu_context.pc = (unsigned long)ret_from_fork;
  p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;

  struct task_struct *last = init;
  while (last->next)
    last = last->next;
  last->next = p;
  preempt_enable();
  return 0;
}
