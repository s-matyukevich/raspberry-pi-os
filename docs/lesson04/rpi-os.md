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

1. We know total amount of memory in the system. It is 1 GB - 1 MB (Last meabute of memory is reserved for device registers) This value is stored in the [HIGH_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L14) constant.
1. First 2 MB of memory are reserved for kernel image and init task stack. This value is stored in [LOW_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L13) constant. All memory allocations starts right after this point. 

### Allocating new task

New task allocation is implemented in [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) function and we are going to examine it in details now. The function starts with disabling preemption and allocationg a pointer for the new task.

```
	preempt_disable();
	struct task_struct *p;
```

Preemption is disabled because we don't want to be rescheduled to a different task in the middle of the `copy__process` function.  

```
	p = (struct task_struct *) get_free_page();
	if (!p)
		return 1;
```

Next new page is allocated. At the bottom of this page we are putting `task_struct` for the newely created task. The rest of this page will be used as task stack.

```
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1; //disable preemtion untill schedule_tail
```

After `task_struct` is alocated we can initialize its properties.  Priority and initial counter are set based on current task priority. State is set to `TASK_RUNNING` inidicating that new task is ready to be started. `preempt_count` is set to 1 indicting that after the task will be executed it should not be rescheduled untill it completes some initialization work.

```
	p->cpu_context.x19 = fn;
	p->cpu_context.x20 = arg;
	p->cpu_context.pc = (unsigned long)ret_from_fork;
	p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
```

This is the most important part of the function. Here `cpu_context` is initialized. Stack pointer is set to the top of the newely allocated  memory page. `pc`  is set to [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L146) function and we need to look at this function now in order to understant why rest of the `cpu_context` registers are initialized in the way they are.

```
.globl ret_from_fork
ret_from_fork:
	bl	schedule_tail
	mov	x0, x20
	blr	x19 		//should never return
```

As you can see `ret_from_fork` first call [schedule_tail](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L65), wich just enabled preemption, and then it calls the function stored in `x19` register with the argument stored in `x20`. (accordingly to ARM calling conventions first argument of a function should be passed in `x0` register). `x19` and `x20` are restored from `cpu_context` just before `ret_from_fork` function is called.

```
	int pid = nr_tasks++;
	task[pid] = p;	
	preempt_enable();
	return 0;
```

Finally `copy_process` adds newely created task to the `task` array and enables preemption for the current task.

Important thing to understand about `copu_process` function is that after it finishes execution no context switch happens. The function only prepares and add new `task_struct` to the `task` array - this task will be executed only after `schedule` function will be called next time.

### Who calls `schedule`?

Before we get into the details of the `schedule` function lets first figure out how `schedule` can be called. There are 2 scenarios.

1. When some task don't have any usefull work to do, but it nevertheless can't be terminates, it can call `schedule` volantirely. That is something `kernel_main` function does.
1. `schedule` is also called on a regular basis from [timer interrupt handler](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/timer.c#L21).

Now let's take a look at [timer_tick](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L70) function, wich is called from timer interrupt.

```
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
```

First of all it decreases corrent task counter. If counter is greater then 0 or prremption is currently disabled the function returns and `schedule` is not called. This implies that counter can go below 0, wich is fixed right after the if statement. Then `schedule` is called with interrupts enabled. (We are inside an interrupt handler, in interrupts are deisabled by default in this case untill you manually enable them). WE will see why interrupts must be enabled during scheduler execution in the next section. 

### Scheduling algorithm

Finally we are ready to look at the scheduler algorithm. I almost precisely copied this algorithm from the first release of the linux kernel. YOu can find the original one [here](https://github.com/zavg/linux-0.01/blob/master/kernel/sched.c#L68)

```
void _schedule(void)
{
	preempt_disable();
	int next,c;
	struct task_struct * p;
	while (1) {
		c = -1;
		next = 0;
		for (int i = 0; i < NR_TASKS; i++){
			p = task[i];
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
	switch_to(task[next]);
	preempt_enable();
}
```

The algorithm works like the following:

1. The first inner `for` loop iterates over all tasks and tries to find a task in `TASK_RUNNING` state with the maximum value of its counter. If such task is found and its counter is greater then 0 we imideately break from the outer `while` loop and switch to the task. If no such task is found this means that no tasks in `TASK_RUNNING`  state curently exist or all such tasks have 0 counters. The first case might happen, for example, when all tasks are waiting for some interrupt. Then second `for` loop is executed. For each task (no mater witch state it is in) this loop increases its counter. Counter incearease is done in a very smart way:

1. The more iterations of the second `for` loop a task passes the more its counter will be increased.
2. A task counter can newer get larger then `2 * priority` 

Then the process is repeated. If there were at least one task in `TASK_RUNNIG` state second iteration of the outer `while` loop will be the last, because after the first iteration all conters are already non 0. However, if no `TASK_RUNNING` tasks are there this process is repeated over and over again untill some of the tasks will move to `TASK_RUNNING` state. But if we are running on a single CPU, how then a task state can cahnge while this loop is running? The answer is that if some task is waiting for an interrupt, this interrupt can happen while `schedule` function is executed and interrupt handler can change the state of the task. This actually explains why interrupts must be enabled during `schedule` execution. This also demonstrates an important distinction between disableing interrupts and disabling prremption. `schedule` disables prremption for the duration of the whole function. This ensures that  nester `schedule` will not be called while we are in the middle of the original function execution. However, interrupts can legaly happen during function execution.

I paid a lot of attension to the situation were some task is waiting for an interrupt, thoug this funcitonality isn't implemented in RPi OS yet. But I still consider it necessry to understand this case because it is a part of the core scheduler algorithm and similar functionality will be adder later. 


### Switching tasks

After the task in `TASK_RUNNING` state with non 0 counter is found [switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L56) function is called. It looks like this.

```
void switch_to(struct task_struct * next) 
{
	if (current == next) 
		return;
	struct task_struct * prev = current;
	current = next;
	cpu_switch_to(prev, next);
}
```

Here we check that next process is not equal to current, and if not `current` variable is updated. The actuall work is redirected to [cpu_switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.S) function.

```
.globl cpu_switch_to
cpu_switch_to:
	mov	x10, #THREAD_CPU_CONTEXT
	add	x8, x0, x10
	mov	x9, sp
	stp	x19, x20, [x8], #16		// store callee-saved registers
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9, [x8], #16
	str	x30, [x8]
	add	x8, x1, x10
	ldp	x19, x20, [x8], #16		// restore callee-saved registers
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]
	mov	sp, x9
	ret
``` 

This is the place were actuall context switch happens. Let's examine it line by line.
 
```
	mov	x10, #THREAD_CPU_CONTEXT
	add	x8, x0, x10
```

`THREAD_CPU_CONTEXT` constant contains offset of `cpu_context` structure in the `task_struct` structure. `x0` ontains pointer to the first artument, wich is current `task_struct`(by curent here I mean the one we are switching from).  After those 2 lines will be executed `x8` will contain pointer to the current `cpu_context`.

```
	mov	x9, sp
	stp	x19, x20, [x8], #16		// store callee-saved registers
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9, [x8], #16
	str	x30, [x8]
```
Next all calle-saved registers are saved in the order in wich they are defined in `cpu_context` struture. `x30` wich is the link register and contains function return address is stored as `pc`, current stack pointer is saved as `sp` and `x29` is saved as `fp` (frame pointer).

```
	add	x8, x1, x10
```

Now `x10` contains offset of the `cpu_context` structure inside `task_struct`, `x1` is a pointer to the next `task_struct`, so `x8` will contain a pointer to the next `cpu_context`.

```
	ldp	x19, x20, [x8], #16		// restore callee-saved registers
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]
	mov	sp, x9
```

Callee saved registers are restored from next `cpu_context`.

```
	ret
```

Function returns to the location pointed to by the link register (`x30`) If we are swithcing to some task for the first time this will be [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L148) function. In all other cases this will be the location, previously saved in the `cpu_context` by the `cpu_switch_to` function.

### How scheduling works with exception entry/exit?

In the previous lesson we have seen how [kernel_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L17) and [kernel_exit](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L4) macros are used to save and restore the process contxt. After scheduler have been introduced a new problem arrives: now it becomes fully legal to enter at interrupt as task 1 and leave it as task 2. This is a problem because `eret` instruction that we are using to return from an interrupts rellies on the fact that return address should be stored in `elr_el1` and processor state in `spsr_el1` registers. So if we want to switch tasks during processing an interrupt we must save and restore those 2 registers alongside with all other general purpose registers. The code that does this is very straightforward, you can find the save part [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L35) and restore [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L35)

### Tracking system state diring context switch

We have already examined all source code related to the context switch. However, that code contains a lot of asyncronos interrupction that makes it difficult to fully understand how the state of the whole system cahnges over time. In this section I want to make this task easier for you - I want to describe the sequence of events that happens from system startup to the time of the second context switch. For each such event I will also include a diagram representing the state of the memory at the time of the event. I hope that such representation will relp you to get a deep understanding of how scheduler works. So , let's start!

1. The kernel is initialized and [kernel_main](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L19) function is executed. Initial stack is configured to start at [LOW_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L13) wich is 4 MB.
    ```
             0 +------------------+ 
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```
1. `kernel_main` calls [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) for the first time. New 4 KB memory page is allocated and `task_struct` is placed at the bottom of this page.
    ```
             0 +------------------+ 
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               | task_struct 1    |
               |------------------|
               |                  |
    0x00401000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```
1.  `kernel_main` calls `copy_process` for the second time and the same process repeats.
    ```
             0 +------------------+ 
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               | task_struct 1    |
               |------------------|
               |                  |
    0x00401000 +------------------+
               | task_struct 2    |
               |------------------|
               |                  |
    0x00402000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```
1. `kernel_main` volantirely calls [schedule](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L21) function and it desides to run task number 1.
1. [cpu_switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.S#L4) saved calee-saved registers in the init task `cpu_context`, wich is located inside kernel image.
1 `cpu_switch_to` restores calee-saved registers from task 1 `cpu_context`. `sp` now points to `0x00401000`, link register to [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L146) function, `x19` contains a pointer to [process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L9) function and `x20` a pointer to string "12345" wich is located somewhere in the kernel image.
1. `cpu_switch_to` calls `ret` instruction withc jums to `ret_from_fork` function.
1. `ret_from_fork` reads `x19` and `x20` registers and  calls `process` function with the argument "12345". After `process` function starts to execute it stack begins to grow.
    ```
             0 +------------------+ 
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               | task_struct 1    |
               |------------------|
               |                  |
               |------------------|
               | task 1 stack     |
    0x00401000 +------------------+
               | task_struct 2    |
               |------------------|
               |                  |
    0x00402000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```
1. A timer interrupt occured. [kernel_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L17) macro saves all general purpose registers + `elr_el1` and `spsr_el1` to the bottom of task 1 stack.
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```
1. `schedule` is called and it decides to run task 2. Task 1 stack continue to grow below task 1 saved registers region. On the diagram this part of the stack is marked as (int), wich means "interrupt stack"
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack (int)     |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```
1. `cpu_switch_to` runs task 2. In order to do this it executes exactly the same sequence of steps that it does for task 1. Task 2 started to execute and it stack grows. Note, that we didn't return from an interrupt at this point, but this is ok, because interrupts now are enabled (interrupts have been enabled previously in [timer_tick](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L70) before `schedule` was called)
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack (int)     |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```
1. Another timer interrupt happens and `kernel_entry` saves all general purpose registers + `elr_el1` and `spsr_el1` at the bottom of task 2 stack. Task 2 interrupt stack begins to grow.
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack (int)     |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack (int)     |
               |------------------------|
               | task 2 saved registers |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```
1. `schedule` is called. It observes that all tasks has their counters set to 0 and set counters to corresponding tasks priorities.
1. `schedule` selects init task to run. (This is because all tasks now have their counters set to 1 and init task is first in the list). But actually, it would be fully legal for `schedule` to select task 1 or task 2 at this point, because their counters are counters has equal values. We are more interested in the case when task 1 is selected so let's now assume that this is what had happend.
1. `cpu_switch_to` is called and it respores previously saved callee-saved  registers from task 1 `cpu_context`. Link register now points [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L63) because this is the place from which `cpu_switch_to` was called last time when task 1 was executed. `sp` points to the bottom of task 1 interrupt stack. 
1. `timer_tick` function resumes execution, starting from [this](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L79) line. It disables interrupts and finally [kernel_exit](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L79) is executed. By the time `kernel_exit` is called task 1 interrupt stack is collapsed to 0.
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack (int)     |
               |------------------------|
               | task 2 saved registers |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```
1. `kernel_exit` restores all general purpose registers as well as `elr_el1` and `spsr_el1`. `elr_el1` now points somewhere in the middle of the `process` function. `sp` points to the bottom of task 1 stack.
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack (int)     |
               |------------------------|
               | task 2 saved registers |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```
1. `kernel_exit` executes `eret` instruction witch uses `elr_el1` register to jump back to `process` function. task 1 resumes it normal execution.

The described above sequence of steps is very important - I personaly consider it one of the most important things in this turtorial. If you have difficulties with understanding it I can advise you to work on the excercise number 1 from this lesson.

### Conclusion

We are done with scheduling, but the processes that our kernel can manage right now are kernel processes - they executed at EL1 and can directly access any kernel functions or data. In the next 2 lessons we are going fix this.
