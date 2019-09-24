## 5.1: User processes and system calls 

We have already added a lot of features to the RPi OS that makes it looks like an actual operating system instead of just a bare metal program. The RPi OS can now manage processes, but there is still a major drawback in this functionality: there is no process isolation at all. In this lesson, we are going to fix this issue. First of all, we will move all user processes to EL0, which restricts their access to privileged processor operations. Without this step any other isolation technics don't make sense, because any user program will be able to rewrite our security settings, thus breaking from isolation. 

If we restrict user programs from direct access to kernel functions, this brings us a different problem. What if a user program needs, for example, to print something to a user? We definitely don't want it to work with the UART device directly. Instead, it would be nice if the OS provides each program with a set of API methods. Such API can't be implemented as a simple set of methods, because each time a user program wants to call one of the API methods current exception level should be raised to EL1. Individual methods in such API are called "system calls", and in this lesson, we will add a set of system calls to the RPi OS.

There is also a third aspect of process isolation: each process should have its own independent view of memory — we are going to tackle this issue in the lesson 6.

### System calls implementation

The main idea behind system calls (syscalls for short) is very simple: each system call is actually a synchronous exception. If a user program need to execute a  syscall, it first has to to prepare all necessary arguments, and then run `svc` instruction. This instruction generates a synchronous exception. Such exceptions are handled at EL1 by the operating system. The OS then validates all arguments, performs the requested action and execute normal exception return, which ensures that the execution will resume at EL0 right after the `svc` instruction. The RPi OS defines 4 simple syscalls: 

1. `write` This syscall outputs something on the screen using UART device. It accepts a buffer with the text to be printed as the first argument. 
1. `clone` This syscall creates a new user thread. The location of the stack for the newly created thread is passed as the first argument.
1. `malloc` This system call allocates a memory page for a user process. There is no analog of this syscall in Linux (and I think in any other OS as well.) The only reason why we need it is that RPi OS doesn't implement virtual memory yet, and all user processes work with physical memory. That's why each process needs a way to figure out which memory page isn't occupied and can be used. `malloc` syscall return pointer to the newly allocated page or -1 in case of an error.
1. `exit` Each process must call this syscall after it finishes execution. It will do all necessary cleanup.

All syscalls are defined in the [sys.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c) file. There is also an array [sys_call_table](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c) that contains pointers to all syscall handlers. Each syscall has a "syscall number" — this is just an index in the `sys_call_table` array. All syscall numbers are defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/sys.h#L6) — they are used by the assembler code to specify which syscall we are interested in. Let's use `write` syscall as an example and take a look at the syscall wrapper function. You can find it [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L4).

```
.globl call_sys_write
call_sys_write:
    mov w8, #SYS_WRITE_NUMBER    
    svc #0
    ret
```

The function is very simple: it just stores syscall number in the `w8` register and generates a synchronous exception by executing `svc` instruction. `w8` is used for the syscall number by convention: registers `x0` — `x7`are used for syscall arguments and `x8` is used to store syscall number, this allows a syscall to have up to 8 arguments. 

Such wrapper functions are usually not included in the kernel itself — you are more likely to find them in the different language's standard libraries, such as [glibc](https://www.gnu.org/software/libc/).

### Handling synchronous exceptions

After a synchronous exception is generated, the [handler](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L98), which is registered in the exception table, is called. The handler itself can be found [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L157) and it starts with the following code.

```
el0_sync:
    kernel_entry 0
    mrs    x25, esr_el1                // read the syndrome register
    lsr    x24, x25, #ESR_ELx_EC_SHIFT // exception class
    cmp    x24, #ESR_ELx_EC_SVC64      // SVC in 64-bit state
    b.eq   el0_svc
    handle_invalid_entry 0, SYNC_ERROR
```

First of all, as for all exception handlers, `kernel_entry` macro is called. Then `esr_el1` (Exception Syndrome Register) is checked. This register contains "exception class" field at offset [ESR_ELx_EC_SHIFT](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/arm/sysregs.h#L46). If exception class is equal to [ESR_ELx_EC_SVC64](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/arm/sysregs.h#L47) this means that the current exception is caused by the `svc` instruction and it is a system call. In this case, we jump to `el0_svc` label and show an error message otherwise. 

```
sc_nr   .req    x25                  // number of system calls
scno    .req    x26                  // syscall number
stbl    .req    x27                  // syscall table pointer

el0_svc:
    adr    stbl, sys_call_table      // load syscall table pointer
    uxtw   scno, w8                  // syscall number in w8
    mov    sc_nr, #__NR_syscalls
    bl     enable_irq
    cmp    scno, sc_nr               // check upper syscall limit
    b.hs   ni_sys

    ldr    x16, [stbl, scno, lsl #3] // address in the syscall table
    blr    x16                       // call sys_* routine
    b      ret_from_syscall
ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR
```

`el0_svc` first loads the address of the syscall table in the `stbl` (it is just an alias to the `x27` register.) and syscall number in the `scno` variable. Then interrupts are enabled and syscall number is compared to the total number of syscalls in the system — if it is greater or equal an error message is shown. If syscall number falls within the required range, it is used as an index in the syscall table array to obtain a pointer to the syscall handler. Next, the handler is executed and after it finishes `ret_from_syscall` is called. Note, that we don't touch here registers `x0` – `x7` — they are transparently passed to the handler.

```
ret_from_syscall:
    bl    disable_irq                
    str   x0, [sp, #S_X0]             // returned x0
    kernel_exit 0
```
`ret_from_syscall` first disables interrupts. Then it saves the value of `x0` register on the stack. This is required because `kernel_exit` will restore all general purpose registers from their saved values, but `x0` now contains return value of the syscall handler and we want this value to be passed to the user code. Finally `kernel_exit` is called, which returns to the user code.

### Switching between EL0 and EL1

If you read previous lessons carefully you might notice a change in the `kernel_entry` and `kernel_exit` macros: now both of them accepts an additional argument. This argument indicates which exception level an exception is taken from. The information about the originating exception level is required to properly save/restore stack pointer. Here are the two relevant parts from the `kernel_entry` and `kernel_exit` macros.

```
    .if    \el == 0
    mrs    x21, sp_el0
    .else
    add    x21, sp, #S_FRAME_SIZE
    .endif /* \el == 0 */
```

```
    .if    \el == 0
    msr    sp_el0, x21
    .endif /* \el == 0 */
```

We are using 2 distinct stack pointers for EL0 and EL1, that's why right after an exception is taken from EL0 the stack pointer is overwritten. The original stack pointer can be found in the `sp_el0` register. The value of this register must be stored and restored before and after taking an exception, even if we don't touch `sp_el0` in the exception handler. If you don't do this you will end up having wrong value in the `sp` register after a context switch. 

You may also ask why don't we restore the value of the `sp` register in the case when an exception was taken from EL1? That is because we are reusing the same kernel stack for the exception handler. Even if a context switch happens during an exception processing, by the time of `kernel_exit`, `sp` will be already switched by the `cpu_switch_to` function. (By the way, in Linux the behavior is different because Linux uses a different stack for interrupt handlers.)

It is also worth noticing that we don't need to explicitly specify to which exception level we need to return before the `eret` instruction. This is because this information is encoded in the `spsr_el1` register, so we always return to the level from which the exception was taken.

### Moving a task to user mode

Before any syscall can take place, we obviously need to have a task running in user mode. There are 2 possibilities how new user tasks can be created: either a kernel thread will be moved to user mode, or a user task can fork itself to create a new user task. In this section, we will explore the first possibility.

The function that actually does the job is called [move_to_user_mode](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/fork.c#Li47), but before we will look into it, let's first examine how this function is used. In order to do so, you need to first open [kernel.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c)  file. Let me copy the relevant lines here.

```
    int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, 0);
    if (res < 0) {
        printf("error while starting kernel process");
        return;
    }
```

First, in the `kernel_main` function we create a new kernel thread. We do this in the same way as we did it in the previous lesson. After the scheduler runs the newly created task,  `kernel_process` function will be executed in kernel mode.   

```
void kernel_process(){
    printf("Kernel process started. EL %d\r\n", get_el());
    int err = move_to_user_mode((unsigned long)&user_process);
    if (err < 0){
        printf("Error while moving process to user mode\n\r");
    } 
}
```

`kernel_process` then prints status message and calls `move_to_user_mode`, passing a pointer to the `user_process` as the first argument. Now let's see what `move_to_user_mode` function is doing.

```
int move_to_user_mode(unsigned long pc)
{
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = pc;
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = get_free_page(); //alocate new user stack
    if (!stack) {
        return -1;
    }
    regs->sp = stack + PAGE_SIZE; 
    current->stack = stack;
    return 0;
}
```

Right now we are in the middle of execution of a kernel thread that was created by forking from the init task. In the previous lesson we've discussed the forking process, and we've seen that a small area (`pt_regs` area) was reserved at the top of the stack of the newly created task. This is the first time we are going to use this area: we will save manually prepared processor state there. This state will have exactly the same format as `kernel_exit` macro expects and its structure is described by the [pt_regs](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/fork.h#L21) struct. 

The following fields of the `pt_regs` struct are initialized in the `move_to_user_mode` function.

* `pc` It now points to the function that needs to be executed in the user mode. `kernel_exit`  will copy `pc` to the `elr_el1` register, thus making sure that we will return to the `pc` address after performing exception return.
* `pstate` This field will be copied to `spsr_el1` by the `kernel_exit` and becomes the processor state after exception return is completed. [PSR_MODE_EL0t](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/fork.h#L9) constant, which is copied to the `pstate` field, is prepared in such a way that exception return will be made to EL0 level. We already did the same trick in the lesson 2 when switching from EL3 to EL1.
* `stack` `move_to_user_mode`  allocates a new page for the user stack and sets `sp` field to point to the top of this page.

`task_pt_regs` function is used to calculate the location of the `pt_regs` area. Because of the way we initialized the current kernel thread, we are sure that after it finished `sp` will point right before the `pt_regs` area. This happens in the middle of the [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L188) function.

```
.globl ret_from_fork
ret_from_fork:
    bl    schedule_tail
    cbz   x19, ret_to_user            // not a kernel thread
    mov   x0, x20
    blr   x19
ret_to_user:
    bl disable_irq                
    kernel_exit 0 
```

As you might notice `ret_from_fork` has been updated. Now, after a kernel thread finishes, the execution goes to the `ret_to_user` label, here we disable interrupts and perform normal exception return, using previously prepared processor state.

### Forking user processes

Now let's go back to the `kernel.c` file. As we've seen in the previous section, after `kernel_process` finishes, [user_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c#L22) function will be executed in the user mode. This function calls `clone` system call 2 times in order to execute [user_process1](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c#L10) function in 2 parallel threads. `clone` system call requires that the location of a new user stack will be passed to it, we also need to call `malloc` syscall in order to allocate 2 new memory pages. Let's now take a look at what the `clone` syscall wrapping function looks like. You can find it [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L22).

```
.globl call_sys_clone
call_sys_clone:
    /* Save args for the child.  */
    mov    x10, x0                    /*fn*/
    mov    x11, x1                    /*arg*/
    mov    x12, x2                    /*stack*/

    /* Do the system call.  */
    mov    x0, x2                     /* stack  */
    mov    x8, #SYS_CLONE_NUMBER
    svc    0x0

    cmp    x0, #0
    beq    thread_start
    ret

thread_start:
    mov    x29, 0

    /* Pick the function arg and execute.  */
    mov    x0, x11
    blr    x10

    /* We are done, pass the return value through x0.  */
    mov    x8, #SYS_EXIT_NUMBER
    svc    0x0
```

In the design of the `clone` syscall wrapping function, I tried to emulate the behavior of the [coresponding function](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/aarch64/clone.S;h=e0653048259dd9a3d3eb3103ec2ae86acb43ef48;hb=HEAD#l35) from the `glibc` library. This function does the following.

1. Saves registers `x0` – `x3`, those registers contain parameters of the syscall and later will be overwritten by the syscall handler.
1. Calls syscall handler.
1. Checks return value of the syscall handler: if it is `0` this means that we return here right after the syscall finishes and we are executing inside the original thread — just return to the caller in this case.
1. If the return value is non-zero, then it is PID of the new task and we are executing inside of the newly created thread.  In this case, execution goes to `thread_start`  label.
1. The function, originally passed as the first argument, is called in a new thread.
1. After the function finishes, `exit` syscall is performed — it never returns.

As you can see, the semantics of the clone wrapper function and clone syscall differ: The former accepts a pointer to the function to be executed as an argument and the later return to the caller twice: first time in the original task and second time in the cloned task.

Clone syscall handler can be found [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c#L11). It is very simple and it just calls already familiar `copy_process` function. This function, however, has been modified since the last lesson — now it supports cloning user threads as well as kernel threads. The source of the function is listed below.

```
int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg, unsigned long stack)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *) get_free_page();
    if (!p) {
        return -1;
    }

    struct pt_regs *childregs = task_pt_regs(p);
    memzero((unsigned long)childregs, sizeof(struct pt_regs));
    memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs * cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE; 
        p->stack = stack;
    }
    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;
    int pid = nr_tasks++;
    task[pid] = p;    
    preempt_enable();
    return pid;
}
```

In case, when we are creating a new kernel thread, the function behaves exactly the same, as was described in the previous lesson. In the other case, when we are cloning a user thread, this part of the code is executed.

```
        struct pt_regs * cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE; 
        p->stack = stack;
```

The first thing that we are doing here is getting access to the processor state, saved by the `kernel_entry` macro. It is not obvious, however, why we can use the same `task_pt_regs` function, which just returns `pt_regs` area at the top of the kernel stack. Why isn't it possible that `pt_regs` will be stored somewhere else on the stack? The answer is that this code can be executed only after `clone` syscall was called. At the time when syscall was triggered the current kernel stack was empty (we left it empty after moving to the user mode). That's why `pt_regs` will always be stored at the top of the kernel stack. This rule will be kept for all subsequent syscalls because each of them will leave kernel stack empty before returning to the user mode.

In the second line current processor state is copied to the new task's state. `x0` in the new state is set to `0`, because `x0` will be interpreted by the caller as a return value of the syscall. We've just seen how clone wrapper function uses this value to determine whether we are still executing as a part of the original thread or a new one.

Next `sp` for the new task is set to point to the top of the new user stack page. We also save the pointer to the stack page in order to do a cleanup after the task finishes.

### Exiting a task

After each user tasks finishes it should call `exit` syscall (In the current implementation `exit` is called implicitly by the `clone` wrapper function.).  `exit` syscall then calls [exit_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sched.c) function, which is responsible for deactivating a task. The function is listed below.

```
void exit_process(){
    preempt_disable();
    for (int i = 0; i < NR_TASKS; i++){
        if (task[i] == current) {
            task[i]->state = TASK_ZOMBIE;
            break;
        }
    }
    if (current->stack) {
        free_page(current->stack);
    }
    preempt_enable();
    schedule();
}
```

Following Linux convention, we are not deleting the task at once but set its state to `TASK_ZOMBIE` instead. This prevents the task from being selected and executed by the scheduler. In Linux such approach is used to allow parent process to query information about the child even after it finishes.  

`exit_process` also deletes now unnecessary user stack and calls `schedule`. After `schedule` is called new task will be selected, that's why this system call never returns.

### Conclusion

Now that the RPi OS can manage user tasks, we become much closer to the full process isolation. But one important step is still missing: all user tasks share the same physical memory and can easily read one another's data. In the next lesson, we are going to introduce virtual memory and fix this issue.

##### Previous Page

4.5 [Process scheduler: Exercises](../../docs/lesson04/exercises.md)

##### Next Page

5.2 [User processes and system calls: Linux](../../docs/lesson05/linux.md)
