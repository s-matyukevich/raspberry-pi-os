#ifndef _ENTRY_H
#define _ENTRY_H

#define S_FRAME_SIZE			272 		// size of all saved registers 
#define S_X0					0			// offset of x0 register in saved stack frame

#define SYNC_INVALID_EL1t		0 
#define IRQ_INVALID_EL1t		1 
#define FIQ_INVALID_EL1t		2 
#define ERROR_INVALID_EL1T		3 

#define SYNC_INVALID_EL1h		4 
#define FIQ_INVALID_EL1h		5 
#define ERROR_INVALID_EL1h		6 

#define FIQ_INVALID_EL0_64		7 
#define ERROR_INVALID_EL0_64	8 

#define SYNC_INVALID_EL1_32		9 
#define IRQ_INVALID_EL1_32		10 
#define FIQ_INVALID_EL1_32		11 
#define ERROR_INVALID_EL1_32	12 

#define SYNC_ERROR				13 
#define SYSCALL_ERROR			14 
#define DATA_ABORT_ERROR		15 

#ifndef __ASSEMBLER__

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		
	"SYNC_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	
	"SYNC_INVALID_EL1_32",		
	"IRQ_INVALID_EL1_32",		
	"FIQ_INVALID_EL1_32",		
	"ERROR_INVALID_EL1_32",	
	"SYNC_ERROR",				
	"SYSCALL_ERROR",			
	"DATA_ABORT_ERROR"		
};

#endif

#endif
