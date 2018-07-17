#ifndef	_USER_SYS_H
#define	_USER_SYS_H

void call_sys_write(char * buf);
int call_sys_fork();
void call_sys_exit();

extern void user_delay ( unsigned long);
extern unsigned long get_sp ( void );
extern unsigned long get_pc ( void );

#endif  /*_USER_SYS_H */
