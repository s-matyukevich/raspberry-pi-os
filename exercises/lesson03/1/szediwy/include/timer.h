#ifndef	_TIMER_H
#define	_TIMER_H

void timer_init ( void );
void handle_timer_irq ( void );

void local_timer_init (void);
void handle_local_timer_irq (void);

#endif  /*_TIMER_H */