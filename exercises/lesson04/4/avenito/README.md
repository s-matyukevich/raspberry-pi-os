# How to implement an unlimited number of tasks

To have an unlimited number of tasks we should have unlimited memory too, but the goal is to implement a way where we can run how many tasks how is possible.

A possible solution is to use a data structure called Threaded Binary Trees. [Here](https://en.wikipedia.org/wiki/Threaded_binary_tree) a reference.

1. Change the task structure in 'sched.h'. We have to change the 'INIT_TASK' too.
1. Create an initial task