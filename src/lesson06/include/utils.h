#ifndef	_UTILS_H
#define	_UTILS_H

extern void DELAY ( unsigned long);
extern void PUT32 ( unsigned long, unsigned int );
extern unsigned int GET32 ( unsigned long );
extern unsigned long GET_EL ( void );
extern unsigned long GET_SP ( void );
extern unsigned long GET_ELR ( void );
extern void set_pgd(unsigned long pgd);
extern unsigned long get_pgd();

#endif  /*_UTILS_H */
