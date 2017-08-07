#ifndef _FORK_H
#define _FORK_H

int copy_process(int nr, unsigned long fn, unsigned long arg);
void ret_from_fork(void);

#endif
