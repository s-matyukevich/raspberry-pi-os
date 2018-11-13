# To use FP/SIMD registers

Remove the "-mgeneral-regs-only" option in Makefile.

# FD/SIMD Registers

To use these registers, we have to store and restore as exactly as we do to the CPU registers.

ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile, avaliable [here](https://developer.arm.com/docs/ddi0487/latest/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile), page 146 describes the 32 registers in the SIMD and floating-point register file, V0-V31.

Cortex -A9 Floating-Point Unit Technical Reference Manual, available [here](https://www.google.com/search?q=Cortex-A9+Floating-Point+Unit+%28FPU%29Technical+Reference+Manual&ie=utf-8&oe=utf-8&client=firefox-b-ab), page 22 we find the "Floating-Point Status and Control Register" description.



