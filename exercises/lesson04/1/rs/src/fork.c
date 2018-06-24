#include "entry.h"
#include "mm.h"
#include "printf.h"
#include "sched.h"

int copy_process(unsigned long fn, unsigned long arg) {
  preempt_disable();
  struct task_struct *p;

  p = (struct task_struct *)get_free_page();
  if (!p)
    return 1;
  p->priority = current->priority;
  p->state = TASK_RUNNING;
  p->counter = p->priority;
  p->preempt_count = 1; // disable preemtion untill schedule_tail

  p->cpu_context.x19 = fn;
  p->cpu_context.x20 = arg;
  p->cpu_context.pc = (unsigned long)ret_from_fork;
  p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
  int pid = nr_tasks++;
  task[pid] = p;
  printf("copy_process: pid=%d, sp=%x\r\n", pid, p->cpu_context.sp);
  preempt_enable();
  return 0;
}
