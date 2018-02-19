#ifndef _ENTRY_H
#define _ENTRY_H

#define S_FRAME_SIZE			272 		// size of all saved registers 
#define S_X0				0		// offset of x0 register in saved stack frame

#define SYNC_INVALID_EL1t		0 
#define IRQ_INVALID_EL1t		1 
#define FIQ_INVALID_EL1t		2 
#define ERROR_INVALID_EL1t		3 

#define SYNC_INVALID_EL1h		4 
#define FIQ_INVALID_EL1h		5 
#define ERROR_INVALID_EL1h		6 

#define FIQ_INVALID_EL0_64		7 
#define ERROR_INVALID_EL0_64		8 

#define SYNC_INVALID_EL0_32		9 
#define IRQ_INVALID_EL0_32		10 
#define FIQ_INVALID_EL0_32		11 
#define ERROR_INVALID_EL0_32		12 

#define SYNC_ERROR			13 
#define SYSCALL_ERROR			14 
#define DATA_ABORT_ERROR		15 

#ifndef __ASSEMBLER__

void ret_from_fork(void);

#endif

#endif
