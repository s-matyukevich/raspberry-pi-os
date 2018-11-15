# To use FP/SIMD registers

Remove the "-mgeneral-regs-only" option in Makefile.

# FD/SIMD Registers

To use these registers, first of all (as in the Exercise 2 - Lesson 2) we have to use the 'cpacr_el1'. See 'boot.S'. 

To switch the tasks we have to store and restore as exactly as we do to the CPU registers.

ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile, available [here](https://developer.arm.com/docs/ddi0487/latest/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile), page 146 describes the 32 registers in the SIMD and floating-point register file, V0-V31.

ARM Cortex-A53 MPCore Processor Advanced SIMD and Floating-point Extension, available [here](http://infocenter.arm.com/help/topic/com.arm.doc.ddi0502e/DDI0502E_cortex_a53_fpu_r0p3_trm.pdf), page 14 describes the processor Advanced SIMD and Floating-point system registers.

# To save the FP/SIMD registers

1. Add a struct to save the registers in 'sched.h': 32 x 128 bits to V0-V31 + Floating-point Control Register (FPCR) + Floating-point Status Register (FPSR).
1. Change also in INIT_TASK.
1. Add code to store and restore the FP/SIMD register in 'sched.S'.


