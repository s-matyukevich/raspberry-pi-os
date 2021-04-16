#include "mm.h"
#include "sysregs.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1
	and	x0, x0, #0x3
	cbz	x0, init_bss
	/* If processor id is not 0 then pending lock processor
	 * (wait for `sev` instruction)
	 */
	wfe
	b	master

proc_hang:
	b 	proc_hang

init_bss:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	sev

	/***********************************************************************/
	/* 	Enable the other cores 											   
	   	link: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/arm64/booting.rst?h=v5.3#n255
	
		The boot loader is expected to enter the kernel on each CPU in the
		following manner:

		- The primary CPU must jump directly to the first instruction of the
		kernel image.  The device tree blob passed by this CPU must contain
		an 'enable-method' property for each cpu node.  The supported
		enable-methods are described below.

		It is expected that the bootloader will generate these device tree
		properties and insert them into the blob prior to kernel entry.

		- CPUs with a "spin-table" enable-method must have a 'cpu-release-addr'
		property in their cpu node.  This property identifies a
		naturally-aligned 64-bit zero-initalised memory location.

		These CPUs should spin outside of the kernel in a reserved area of
		memory (communicated to the kernel by a /memreserve/ region in the
		device tree) polling their cpu-release-addr location, which must be
		contained in the reserved region.  A wfe instruction may be inserted
		to reduce the overhead of the busy-loop and a sev will be issued by
		the primary CPU.  When a read of the location pointed to by the
		cpu-release-addr returns a non-zero value, the CPU must jump to this
		value.  The value will be written as a single 64-bit little-endian
		value, so CPUs must convert the read value to their native endianness
		before jumping to it.

		- CPUs with a "psci" enable method should remain outside of
		the kernel (i.e. outside of the regions of memory described to the
		kernel in the memory node, or in a reserved area of memory described
		to the kernel by a /memreserve/ region in the device tree).  The
		kernel will issue CPU_ON calls as described in ARM document number ARM
		DEN 0022A ("Power State Coordination Interface System Software on ARM
		processors") to bring CPUs into the kernel.

		The device tree should contain a 'psci' node, as described in
		Documentation/devicetree/bindings/arm/psci.yaml.

		- Secondary CPU general-purpose register settings
		x0 = 0 (reserved for future use)
		x1 = 0 (reserved for future use)
		x2 = 0 (reserved for future use)
		x3 = 0 (reserved for future use)
	*/

	/* cpu0: cpu@0 {
			device_type = "cpu";
			compatible = "arm,cortex-a72";
			reg = <0>;
			enable-method = "spin-table";
			cpu-release-addr = <0x0 0x000000d8>;
		};

		cpu1: cpu@1 {
			device_type = "cpu";
			compatible = "arm,cortex-a72";
			reg = <1>;
			enable-method = "spin-table";
			cpu-release-addr = <0x0 0x000000e0>;
		};

		cpu2: cpu@2 {
			device_type = "cpu";
			compatible = "arm,cortex-a72";
			reg = <2>;
			enable-method = "spin-table";
			cpu-release-addr = <0x0 0x000000e8>;
		};

		cpu3: cpu@3 {
			device_type = "cpu";
			compatible = "arm,cortex-a72";
			reg = <3>;
			enable-method = "spin-table";
			cpu-release-addr = <0x0 0x000000f0>;
		}; */
	/****************************************************/	
	mov x0, #0
	adr x0, configure_el1

	mov x1, #0xe0
	str x0, [x1]
	mov x1, #0xe8
	str x0, [x1]	
	mov x1, #0xf0
	str x0, [x1]

configure_el1:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0


	ldr x0, =CPACR_EL1_MASK
	msr cpacr_el1, x0

  	ldr    x0, =SPSR_VALUE
    msr    spsr_el2, x0

	adr    x0, master
    msr    elr_el2, x0

	eret

master:
	mrs	x0, mpidr_el1
	and	x0, x0, #0x3

	mov	x1, #SECTION_SIZE
	mul	x1, x1, x0
	add	x1, x1, #LOW_MEMORY
	mov	sp, x1

	bl	kernel_main
	b 	proc_hang	

