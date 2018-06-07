## 5.3: Exercises

1. When a task is executed in a user mode, try to access some of the system registers. Make sure that a synchronous exception is generated in this case. Handle this exception, use `esr_el1` register to distinguish it from a system call.
1. Implement a new system call that can be used to set current task priority. Demonstrate how priority changes are dynamically applied while the task is running. 
1. Adapt lesson 05 to run on qemu. Check [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue for reference.
