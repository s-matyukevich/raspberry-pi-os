#ifndef	_IRQ_H
#define	_IRQ_H

void enable_interrupt_controller( void );

void enable_interrupt(unsigned int irq);
void assign_target(unsigned int irq, unsigned int cpu);

void irq_vector_init( void );
void enable_irq( void );
void disable_irq( void );

#endif  /*_IRQ_H */