# Task Priority

To assign priority to the task, the new parameter 'pri' was added.

int copy_process(unsigned long fn, unsigned long arg, unsigned int pri) 

# Results

Assigning priority, is possible to check that one task print much more characters than the other.

(Priority 3) task output: 345123451234512345123451234512 
(Priority 1) task output: eabcde 

