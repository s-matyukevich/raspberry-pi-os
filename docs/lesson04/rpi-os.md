## Lesson 4: Scheduler 

By now PRi OS is already a fairy complicated bare metall programm, but to be onest we still can't call it an operating system. The reason is that it can't do any of the core tasks that any OS should do. One of such core tasks is called process scheduling. By scheduling I mean that nn operating system should be able to share CPU time between different processes. The hard part of it is that all running processes should be unaware of the scheduling happening - each of them should view itself as the only process ocupying  CPU. In this lesson we are going to add this functionality to the RPi OS.

### task_struct

If we want to manage processes, the first thing we should do is to create a structure that descibes a process. In linux such structure is called `task_struct` and it describes both processes and threds (in linux both of them are just different types of tasks). As we are mostly mimicing linux implementation we are going to do the same. RPi OS [task_struct](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L36) looks like the following.

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
* `state` - this is the state of the currently runnint task. For tasks that are just doing some work using CPU the state would always be [TASK_RUNNING](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L15). Actually this is the only state that RPi OS is going to support for now. However later we will have to add a few additional states. For example, a task that is waiting for a particular interrupt to happen should be moved to different state, because it don't make sense to awaken it while needed interrupt haven't yet happen.
* `counter` - this field is used ot determine how long current task have been running. `counter` decreases by 1 each timer tick and when it reaches 0 another task is scheduled.
* `priority` - whent new task is scheduled its `priority` is copied to `counter`. By setting tasks priority we can regulate the amount of processor time thet the task gets relative to other processes.
* `preempt_count` - if this field has non 0 value it is an indicator that right now current task is executing some critical function that must not be interrupted (for example, it runs a scheduling function) If timer tick occured at such time it is ignored and rescheduling is not triggered.

After kernel startup there is only one task running: the one that runs [kernel_main](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L19) function, it is called "init task". Before scheduler functionality will be enabled we must fill `task_struct` corresponding to this task. This is done [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L53). 

All tasks are stored in [task](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L53) array. This array has only 64 slots - that is the maximum number of simultanious tasks that we can have in RPi OS. It is definetely not the best solution for the production ready OS, but it is ok for our goals.

There is also a very impotant variable called [current](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L6) that always points to the currently executing task. Both `current` and `task` array from the beggining are initialized with the init task. There is also a global variable called [nr_tasks](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L8) - it contains number of currently running tasks in the system.

That are all structures and global variables that we are going to use to implement scheduler functionality. In the description of the `task_struct` I already brefly mentioned some aspects of how scheduling works, because it is imposible to understand the meaning and purpose of a particular `task_struct` field without understanding how this field is used. Now we are going to examine the scheduing algorith in much more details and we will start with the main function.

### Kernel main function

Before we will dig into scheduler implementation I want to quickly show you how we are going to prove that scheduler actually works. In order to understand that you need to take a look at [kernel.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c) file. Let me copy the relevant content here.

```
void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_irq();

	int res = copy_process((unsigned long)&process, (unsigned long)"12345");
	if (res != 0) {
		printf("error while starting process 1");
		return;
	}
	res = copy_process((unsigned long)&process, (unsigned long)"abcde");
	if (res != 0) {
		printf("error while starting process 2");
		return;
	}

	while (1){
		schedule();
	}	
}
```

There are a few important things about this code.

1. New function [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) has beed introduced. This function takes 2 arguments: a function to execute in a new thread and an argument that need to be passed to this function. `copy_process` alocates a new `task_struct`  and makes it available for the scheduler.
1. Another new function for us is called [schedule](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L21). This is the core scheduler function - it checks whether there is a new task that need to be executed. A task can volantirely call `schedule` if it doesn't have any work to do at the momnt. `schedule` is also called from the timer interrupt handler.

We are calling `copy_process` 2 times each time passing a pointer to the [process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L9) function as the first argument. `process` function is very simple.

```
void process(char *array)
{
	while (1){
		for (int i = 0; i < 5; i++){
			uart_send(array[i]);
			delay(100000);
		}
	}
}
``` 

It just keep printing on the screen characters from the array, that is passed as an argument to the `process` function. First time it is called with the argument "12345" and second time the argument is "abcde". If our scheduler implementation is correct we should see on the screen combined output from both functions. 

### Memory allocation

Each task in the system should have its own dedicated stack. That's why when alocating new task we mast have a way of allocating memory. For now our memory allocator is extrimely primitive. (The implementation can be found in [mm.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/mm.c) file)

```
static unsigned short mem_map [ PAGING_PAGES ] = {0,};

unsigned long get_free_page()
{
	for (int i = 0; i < PAGING_PAGES; i++){
		if (mem_map[i] == 0){
			mem_map[i] = 1;
			return LOW_MEMORY + i*PAGE_SIZE;
		}
	}
	return 0;
}

void free_page(unsigned long p){
	mem_map[p / PAGE_SIZE] = 0;
}
```
The allocator can work only with memory pages (each page is 4 KB in size). There is an array called `mem_map` that for each page in the system holds its status: whether it is allocated of free. Whenever we need to allocate new page we just loop through this array and return first analocated page. This implementation is based on 2 assumptions: 

1. We know total amount of memory in the system. It is 1 GB - 1 MB (Last meabute of memory is reserved for device registers) This value is stored in the [HIGH_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L4) constant.
1. First 2 MB of memory are reserved for kernel image and init task stack. This value is stored in [LOW_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L4) constant 
