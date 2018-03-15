## Lesson 5: Exercises

1. When a task is executed in a user mode try to access some of the system registers. Make sure that a syncronos exception is generated in this case. Handle this exception, use `esr_el1` register to distigush it from a system call.
1. Implement a new system call that can be used to set current task priority. Demonstrate how priority changes are dynamically applied while task is running. 
