#include "mm.h"

//Make sure the code goes into the .text.boot section
.section ".text.boot"

.globl _start
_start:
	/*Move to ARM reg from coprocessor reg or special reg in this case
	multiprocessor affinity register */

	mrs	x0, mpidr_el1		//Read mpidr_el1 into x0

	/*[7:0]	Aff0	Affinity level 0. Lowest level affinity field.
	Indicates the core number in the Cortex-A53 processor. The possible values are:
	0x0    A cluster with one core only.
	0x0, 0x1    A cluster with two cores.
	0x0, 0x1, 0x2    A cluster with three cores.
	0x0, 0x1, 0x2, 0x3    A cluster with four cores.*/

	and	x0, x0, #0xFF		//Check processor id

	//Compare and branch on zero so all cores which will not be used get trapped
	cbz	x0, master		  //And the one to be used will branch to master
	b	proc_hang

	//Infinite loop to trap the processors which we are not going to use (yet)
proc_hang:
	b 	proc_hang

	//Functio to calculate and call a function to clean the bss section
master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	//After cleaning we set the stack pointer and pass execution to kernel_main
	mov	sp, #LOW_MEMORY
	bl	kernel_main
	b 	proc_hang		//Should never come here
