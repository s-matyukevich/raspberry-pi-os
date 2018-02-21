## Lesson 4: Scheduler 

By now PRi OS is already a fairy complicated bare metall programm, but to be onest we still can't call it an operating system. The reason is that it can't do any of the core tasks that any OS should do. One of such core tasks is called process scheduling. By scheduling I mean that nn operating system should be able to share CPU time between different processes. The hard part of it is that all running processes should be unaware of the scheduling happening - each of them should view itself as the only process ocupying  CPU. In this lesson we are going to add this functionality to the RPi OS.

### task_struct

If we want to manage processes, the first thing we should do is to create a structure that descibes a process. In linux such structure is called `task_struct` and it describes both processes and threds (in linux both of them are just different types of tasks). As we are mostly mimicing linux implementation we are going to do the same. RPi OS [task_struct](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L41) looks like the following.

```
struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

struct task_struct {
	struct cpu_context cpu_context;
	long state;	
	long counter;
	long priority;
	long preempt_count;
};
```

This struct has the following members:

* `cpu_context` this is an embedded structure that contains values of all registers that might be different between switching tasks. A reasonable question to ask is why we save not all registers, but only registers `x19 - x30`? (`fp` is `x29` and `pc` is `x30`, those two registers have spceial meaning) The answer is that actuall context switch happens only  when a task calls [cpu_switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.S#L4) function. So from the point of view of the task that is being switched, it just calls `cpu_switch_to` function and it returns after some time, the task don't notice that another task happens to runs during this period.  Accordingly to ARM calling conventions registers `x0 - x18` can be overritten by the called function, so the caller must not assume that the values of those registers will surwife after a function call. That's why it don't make sense to save `x0 - x18` registers.
* `state` - 
