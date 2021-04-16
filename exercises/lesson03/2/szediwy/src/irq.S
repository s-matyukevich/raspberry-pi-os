.globl irq_vector_init
irq_vector_init:
	adr	x0, vectors		    // load VBAR_EL1 with virtual
	msr	vbar_el1, x0     
	ret

.globl enable_irq
enable_irq:
	msr daifclr, #0b0010    
	ret

.globl disable_irq
disable_irq:
	msr	daifset, #0b0010
	ret