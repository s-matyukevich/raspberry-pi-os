#ifndef	_UTILS_H
#define	_UTILS_H

extern void delay ( unsigned long);
extern void put32 ( unsigned long, unsigned int );
extern unsigned int get32 ( unsigned long );
extern int get_el ( void );

extern void set_fpsimd_reg0(unsigned int);
extern void set_fpsimd_reg2(unsigned int);
extern void set_fpsimd_reg31(unsigned int);
extern unsigned int get_fpsimd_reg0(void);
extern unsigned int get_fpsimd_reg2(void);
extern unsigned int get_fpsimd_reg31(void);

#endif  /*_UTILS_H */
