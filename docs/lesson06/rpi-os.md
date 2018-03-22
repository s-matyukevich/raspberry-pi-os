## Lesson 4: Virtual memory management 

The RPi OS now can run and schedule user processes, but the isolation between them is not complete - all processes and the kernel itself share the same memory. This allows any process to easily access somebody else's data and even kernel data. And even if we assume that there all our processes are not malicios there is another drawback: before allocating memory each process need to know which memory regions are already occupied - this makes memory allocation more comlicated.

### Translation process

In this lesson we are going to fix all above mentioned issues by introducing virtual memory. Virtual memory provides each process with an abstraction wich makes it think that it ocupies all available memory. Each time a process need to access some memory location it uses virtual address, wich is translated into physical address. The process of translation is done completely transparent for the process and is performed by a special device: MMU (Memory Mapping Unit). The MMU uses translation tables in order to translate virtual address into physicall address. The process of translation is described by the following diagram.

```
                           Virtual address                                                                 Physical Memory
+-----------------------------------------------------------------------+                                +-----------------_+
|         | PGD Index | PUD Index | PMD Index | PTE Index | Page offset |                                |                  |
+-----------------------------------------------------------------------+                                |                  |
63        47     |    38      |   29     |    20    |     11      |     0                                |     Page N       |
                 |            |          |          |             +--------------------+           +---->+------------------+
                 |            |          |          +---------------------+            |           |     |                  |
          +------+            |          |                                |            |           |     |                  |
          |                   |          +----------+                     |            |           |     |------------------|
+------+  |        PGD        |                     |                     |            +---------------->| Physical address |
| ttbr |---->+-------------+  |           PUD       |                     |                        |     |------------------|
+------+  |  |             |  | +->+-------------+  |          PMD        |                        |     |                  |
          |  +-------------+  | |  |             |  | +->+-------------+  |          PTE           |     +------------------+
          +->| PUD address |----+  +-------------+  | |  |             |  | +->+--------------+    |     |                  |
             +-------------+  +--->| PMD address |----+  +-------------+  | |  |              |    |     |                  |
             |             |       +-------------+  +--->| PTE address |----+  +-------------_+    |     |                  |
             +-------------+       |             |       +-------------+  +--->| Page address |----+     |                  |
                                   +-------------+       |             |       +--------------+          |                  |
                                                         +-------------+       |              |          |                  |
                                                                               +--------------+          +------------------+
```

The following facts are critical to understand this diagram and memory translation process in general.

* Memory for a process is always allocated only in pages. Page is a contignious memory region 4KB in size (ARM processors support larger pages, but 4KB is the most comon casee and we are going to limit our discussion only to this page size)
* Page tables have hierarchical structure. An Item in each of the tables contains an address of the next table in the hierarchy 
* There are 4 levels in the table hierarchy: PGD (Page Global Directory), PUD (Page Upper Directory), PMD (Page Middle Directory), PTE (Page Table Entry). PTE is the last table in the hierarchy and it pointes to the actuall page in the physical memory.
* Memory translation process starts by locating the address of PGD (Page Global Directory) table. The address of this table is stored in the `ttbr0_el1` register. Each process has its own copies of all page tables, including PGD, and therefore each process must keep its PGD address. During a context switch PGD address of the next process is loaded into the `ttbr0_el1`  register.
* Next MMU uses PGD pointer and virtual address to calculate coresponding physical address. All virtual addresses uses only 48 out of 64 available bits.  When doing translation MMU splits an address into 4 parts.
  * Bits [39 - 47] contain an index in the PGD table. MMU uses this index to find the location of the PUD.
  * Bits [30 - 38] contain an index in the PUD table. MMU uses this index to find the location of the PMD.
  * Bits [21 - 29] contain an index in the PMD table. MMU uses this index to find the location of the PTE.
  * Bits [12 - 20] contain an index in the PTE table. MMU uses this index to find a page in the physical memory.
  * Bits [0 - 11] contain an offsetin in the physical page. MMU uses this offset to determine the exact position in the previously found page that correspond to the original virtual address.

Now let's make a small exercise and calculate the size of a page table. From the diagram above we know that index in a page table occupies 9 bits (This is true for all page table levels) This means that each page table contains `2^9 = 512` items. Each item in a page table is an address of either next page table in the hierarhy or a physical page in case of PTE. As we are using 64 bit processor each address mush be 64 bit or 8 bytes in size. Putting all of this togeter we can calculate that the size of a page table must be `512 * 8 = 4096` bytes or 4 KB. But this is exactly the size of a page! 

### Section mapping

There is one more thing that I want to discuss befor we start looking at the source code - section mapping. Sometimes there is a need to map large parts of contignious physical memory. In this case instead of 4 KB pages we can directly map 2 MB blocks that are called sections. This allows to eliminate 1 level of translation. The translation diagram in this case looks like the following.

```
                           Virtual address                                               Physical Memory
+-----------------------------------------------------------------------+              +-----------------_+
|         | PGD Index | PUD Index | PMD Index |      Section offset     |              |                  |
+-----------------------------------------------------------------------+              |                  |
63        47     |    38      |   29     |    20            |           0              |    Section N     |
                 |            |          |                  |                    +---->+------------------+
                 |            |          |                  |                    |     |                  |
          +------+            |          |                  |                    |     |                  |
          |                   |          +----------+       |                    |     |------------------|
+------+  |        PGD        |                     |       +------------------------->| Physical address |
| ttbr |---->+-------------+  |           PUD       |                            |     |------------------|
+------+  |  |             |  | +->+-------------+  |            PMD             |     |                  |
          |  +-------------+  | |  |             |  | +->+-----------------+     |     +------------------+
          +->| PUD address |----+  +-------------+  | |  |                 |     |     |                  |
             +-------------+  +--->| PMD address |----+  +-----------------+     |     |                  |
             |             |       +-------------+  +--->| Section address |-----+     |                  |
             +-------------+       |             |       +-----------------+           |                  |
                                   +-------------+       |                 |           |                  |
                                                         +-----------------+           |                  |
                                                                                       +------------------+
```

As you can see the difference here is that now PMD contains a pointer to the physical section. Also offset occupies 21 bit instead of 12 bit (this is because we need 21 bit to encode offset from 0 to 2MB) 

### Page descriptor format

You may ask how does the MMU know whether PMD item points to a PTE of to a pysicall 2 MB section? In order to answer this question wee need to take a closer look at the structure of a page table item. Now I can confess that I wasn't quate accurate when claiming that an intem in a page table always contains an address of either next page table or a physical page: each such item includes some other information as well. An item in a page table is called "descriptor". A description has a special format, wich is described below.


```
                           Descriptor format                               
`+------------------------------------------------------------------------------------------+
 | Upper attributes | Address (bits 47:12) | Lower attributes | Block/table bit | Valid bit |
 +------------------------------------------------------------------------------------------+
 63                 47                     11                 2                 1           0 
```

The key thing to understand here is that each descriptor always points to something that is page alligned (either a physical page, a section or the next page table in the hierarchy). This means that last 12 bits of the address, stored in descriptor, will always be 0. This also means that MUU can use those bits to store something more usefull - and that is exactly what it does. Now let me explain the meaning of all bits in a descriptor.

* **Bit 0**. This bit must be set to 1 for all valid descriptors. If MMU encounter non valid descriptor during translation process a syncronos exception is generated. Then kernel then should handle this exception, allocate new page and prepare correct descriptor (We will look in details on how this works a little bit later)
* **Bit 1**. This bit indicates whether curent descriptor points to a next page table in the hierarchy (we call such descriptor a "table descriptor") ot it points instead to a physical page or a section (such descriptors are called "block descriptors").
* **Bits [11:2]**. Those bits are ignored for table descriptors. For block descriptos they contain some attributes that controls, for example, whether the mapped page is cachable, executable, etc.
* **Bits [47:12]**. This is the place were the address that a descriptor points to is stored. As I mentioned previously, only bits [47:12] of the address need to be stored, because all other bits are always 0.
* **Bits [63:48] Another set of attributes.

### Configuring page attributes

As I mentioned in the previous section, each block descriptor contains a set of attributes that controls various virtual page parameters. However, the attributes that are most important for our discussion are not configured directly in the descriptor. instead ARM processors implement a trich, wich allow them to save some spase in the descriptor attributes section.

ARM.v8 srchitecture introduces `mair_el1` register. This register consist of 8 sections 8 bits in lengs. Each such section configures a common set of attributes. A descriptor then specifies just an indext of the `mair`section, instead of specifying all attributes directly. This allows to use only 2 bits in the descriptor to reference required `mair` section. The meaning of each bith in the `mair` section is described at the page 1990 of the `AArch64-Reference-Manual`. In the RPi OS we are using only a few of availabe attribute options. [Here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/arm/mmu.h#L11)  is conde that prepares values for the `mair` register.

```
/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]
 *			n	MAIR
 *   DEVICE_nGnRnE	000	00000000
 *   NORMAL_NC		001	01000100
 */
#define MT_DEVICE_nGnRnE 		0x0
#define MT_NORMAL_NC			0x1
#define MT_DEVICE_nGnRnE_FLAGS		0x00
#define MT_NORMAL_NC_FLAGS  		0x44
#define MAIR_VALUE			(MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC))
```

Here we are using only 2 out of 8 available slots in the `mair` registers. The first one coresponds to device memory and second to normal non cachable memory. `MT_DEVICE_nGnRnE` and `MT_NORMAL_NC` are indexes that we are going to use in block descriptors, `MT_DEVICE_nGnRnE_FLAGS` and `MT_NORMAL_NC_FLAGS` are values that we are storing in the first 2 slots of the `mair_el1` register.

### Kernel vs user virtual memory

After the MMU is switched on each memory access must use virtual memory instead on physical memory. One consequence of this fact is that kernel itself must be prepared to use virtual memory and maintain its own set of page tables. One posible solution could be to to reload `pgd`  register each time we switch from user to kernel mode. The problem is that switching `pgd` is very expensive operation because it requires invalidating all caches. Having in mind how often we need to switch from user mode to kernel mode, this solution would make caching completely useless and therefore this solution is never used in the OS development. What operating system are doing instead is splitting address space into 2 parts: user space and kernel space. 32 bit architectures usually allocate first 3 GB of the address space for user programs and reserve last 1 GB for the kernle. 64 bit architectures are much more faivorable in this regard because of their huge address space. And even more: ARM.v8 architecture comes with a native feature that can be used to easily implement user/kernel address split.

There are 2 register that can hold the address of the PGD: `ttbr0_el1` and `ttbr1_el1`. As you might remember we are using only 48 bits in the addresses out of 64 availabe, so the upper 16 bits can be used to distigush between `ttbr0`  and `ttbr1` translation processes. If upeer 16 bits are all equal to 0 then PGD address stored in `ttbr0_el1` is used, and it the address starts with `0xffff`(first 16 bit are all equal to 1) then PGD address stored in the `ttbr1_el1` is selected. The architecture also ensures that a process running at EL0 can never access virtual addresses started with `0xffff` without generating a syncronos exception.  From this description you can easily infer that a pointer to the kernel PGD is stored in the `ttbr1_el1` and is kept there throughout the life of the kernel, and `ttbr0_el1` is used to store current user process PGD.

On implication of this aproach is that all absolute kernel addreses must start with `0xffff`. There are 2 place in the RPi OS source code, were we handle this. In the [linker script](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/linker.ld#L3) we specify base address of the image as `0xffff000000000000`. This will make the compiler think that our image is going to be loaded at `0xffff000000000000` address, and therefore whenever it needs to generate an absolute address it will make it right. (There are a few more changes to the linker script, but we will discuss them later)

There is one more place were we hardcode absolute addresses: in the [header](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/peripherals/base.h#L7) were we define device base address. Now we will access all device memory startgin from `0xffff00003F000000` Certainly, that in order for this to work we need to first map all memory, wich kernel need to access In the next section we will explore in detail the code that creates this mappling.

### Initializing kernel page tables

The process of creating kernel page tables is something that we need to handle very early in the boot process. It starts in the [boot.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L42) file. Right after we switch to EL1 and cleared the BSS [__create_page_tables](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L92) function is called. Let's examine it line by line.

```
__create_page_tables:
	mov	x29, x30						// save return address
``` 

First the function saves `x30` (link register). As we are going to call other functions from `__create_page_tables`, `x30` will be overriten. Usually `x30` is saved on the stack, but as we know that we are not going to use recursion and nobody else will use `x29` during `__create_page_tables` execution this simple method of preservind link register also works fine.

```
	adrp	x0, pg_dir
	mov	x1, #PG_DIR_SIZE
	bl 	memzero
```

Next we clear initial page tables area. Inportant things to understand here is were this area is located and how dow we know its size? Initial page tables area is defined in the [linker script](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/linker.ld#L20) - this means that we are alocating the spot for this area in the kernel image itself. Calculating the size of this area is a little bit more trickier. First we need to understand the structure of the initial kernel page tables. We know that all our mappings are all inside 1 GB region (this is the size of RPi memory). One PGD descriptor can cover `2^39 = 512 GB`  and one PUD descriptor can cover `x^30 = 1 GB` of contignious virtual mapping area. (Those values are calculated based on the PGD and PUD indexes location in the virtual address) This means that we need just one PGD and one PUD to map the whole RPi memory, and even more - both PGD and PUD will contain a simgle descriptor. If we have a single PUD entry there also must be a single PMD table, to wich this table will point. (Single PMD entry covers 2 MB, there are 512 items in a PMD, so in total the whole PMD table covers the same 1 GB of memory that is covered by a single PUD descriptor) 
Next, we know that we need to map 4 MB region starting at physical offset 0 and 16 MB region at physical offset `0x3F000000`. The sizes of bosh regions are multiples of 2 MB so we can use section mapping - this means that we don't need PTE att all. So in total we need 3 pages: one for PGD, PUD and PTE - this is precisely the size of the initial page table area.- 

Now we are going to step outside `__create_page_tables` function and take a look on 2 essential macros: [create_table_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L68) and [create_block_map](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L77)

`create_table_entry`s responsible for allocating a new page table (In our case eiter PGD or PUD) The source code is listed below.

```
	.macro	create_table_entry, tbl, virt, shift, tmp1, tmp2
	lsr	\tmp1, \virt, #\shift
	and	\tmp1, \tmp1, #PTRS_PER_TABLE - 1			// table index
	add	\tmp2, \tbl, #PAGE_SIZE
	orr	\tmp2, \tmp2, #MM_TYPE_PAGE_TABLE	
	str	\tmp2, [\tbl, \tmp1, lsl #3]
	add	\tbl, \tbl, #PAGE_SIZE					// next level table page
	.endm
``` 

This macro accepts the following arguments

* `tbl` - pointer to a memory region were new table has to be allocated.
* `virt` - virtual address that we are currently mapping.
* `shift` - shift that we need to apply to the virtuall address in order to extract current table index. (39 in case of PGD and 30 in case of PUD) 
* `tmp1`, `tmp2` - temporary registers.

This macro is very important, so we are going to spend some time understanding it.

```
	lsr	\tmp1, \virt, #\shift
	and	\tmp1, \tmp1, #PTRS_PER_TABLE - 1			// table index
```

The first two lines of the macro are responsible for extracting table index from the virtual address. We are applying left shift firt to strip everything to the right of the index, and then using `and` operation to strip everything to the left.

```
	add	\tmp2, \tbl, #PAGE_SIZE
```

Then address of the next page table is calculated. Here we are using the convention that all our initial page tables are located in one contignious memory region. We simply asume that the next page table in the hierarchy will be adjusend to the current page table. 

```
	orr	\tmp2, \tmp2, #MM_TYPE_PAGE_TABLE	
```

Next a pointer to the next tage table in the hierarchy is converted to a table descriptor. (A descriptor mast have 2 lower bits set to `1`)

```
	str	\tmp2, [\tbl, \tmp1, lsl #3]
```

Then the descriptor is stored in the current page table. We use previously calculated index to find the right spot in the table.

```
	add	\tbl, \tbl, #PAGE_SIZE					// next level table page
```

Finally we change `tbl` parameter to point to the hext page table in the hierarchy. This is convenient because now we can call `create_table_entry` one more time for the next table in the hierarchy without making any adjustments to the `tbl` parameter. This is precisely what we are doing in the [create_pgd_entry]((https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L63) mcro, withc wi just a wrapper that allocates both PGD and PUD. 

Next important macro is`create_block_map` As you might guess this macro is responsible for populating entries of the PMD table. It looks like the following.

```
	.macro	create_block_map, tbl, phys, start, end, flags, tmp1
	lsr	\start, \start, #SECTION_SHIFT
	and	\start, \start, #PTRS_PER_TABLE - 1			// table index
	lsr	\end, \end, #SECTION_SHIFT
	and	\end, \end, #PTRS_PER_TABLE - 1				// table end index
	lsr	\phys, \phys, #SECTION_SHIFT
	mov	\tmp1, #\flags
	orr	\phys, \tmp1, \phys, lsl #SECTION_SHIFT			// table entry
9999:	str	\phys, [\tbl, \start, lsl #3]				// store the entry
	add	\start, \start, #1					// next entry
	add	\phys, \phys, #SECTION_SIZE				// next block
	cmp	\start, \end
	b.ls	9999b
	.endm
```

Parameters here are a little bit different.

* `tbl` - a pointer to the PMD table.
* `phys` - start of the physical region to be mapped
* `start` - virtual address of the first section to be mapped. 
* `end` - virtual address of the last section to be mapped. 
* `flags` - flags that need to be copied into lower attributes of the block descriptor.
* `tmp1` - temporary register.

Now, let's examine the source.

```
	lsr	\start, \start, #SECTION_SHIFT
	and	\start, \start, #PTRS_PER_TABLE - 1			// table index
```

Those 2 lines extract table index from `start` virtual address. This is done exactly in the same way as we did it before in the `create_table_entry` macro.

```
	lsr	\end, \end, #SECTION_SHIFT
	and	\end, \end, #PTRS_PER_TABLE - 1				// table end index
```

The same thing is repeated for the `end` address. Now both `start` and `end` contains not virtuall addresses but indexes in the PMD table, corresponding to the orriginal addresses.

```
	lsr	\phys, \phys, #SECTION_SHIFT
	mov	\tmp1, #\flags
	orr	\phys, \tmp1, \phys, lsl #SECTION_SHIFT			// table entry
```

Next, block descriptor is prepared and stored in the `tmp1` variable. In order to prepare the descriptor `phys` parameter is first shifedt to right then shifted back and merged with the `flags` parameter using `orr` instruction. If you wonder why do we have to shift the address back and forht - the answer is that this cleares first 21 bit in the `phys` address and makes our macro universal, allowing it to be used with any address, not just the first address of the section. 

```
9999:	str	\phys, [\tbl, \start, lsl #3]				// store the entry
	add	\start, \start, #1					// next entry
	add	\phys, \phys, #SECTION_SIZE				// next block
	cmp	\start, \end
	b.ls	9999b
```

The finall part of the function is executed inside a loop. Here we first store current descriptor at the right index in the PMD table. Next we increase current index by 1 and update the descriptor to point to the next section. We repeat the same process untill current index becomes equal last index.

Now, when you understand how `create_table_entry` and `create_block_map` macros works, it will be very easy to understand the rest of the `__create_page_tables` function.

```
	adrp	x0, pg_dir
	mov	x1, #VA_START 
	create_pgd_entry x0, x1, x2, x3
```

Here we create both PGD and PUD. We configure them to start mapping from [VA_START](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/mm.h#L6) virtuall address. Because of the semantics of the `create_table_entry` macro, after `create_pgd_entry`  finishes `x0` will contain the address of the next table in the hierarchy - namely PMD.

```
	/* Mapping kernel and init stack*/
	mov 	x1, xzr							// start mapping from physical offset 0
	mov 	x2, #VA_START						// first virtual address
	ldr	x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)		// last virtual address
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4
```

Next, we create virtuall mapping of the whole memory, excluding device registers region. We use [MMU_FLAGS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/arm/mmu.h#L24) constant as `flags` parameter - this makes all sections to be mapped as normal noncacheable memory. (Note, that `MM_ACCESS` flag is also specified as part of `MMU_FLAGS` constant. Without this flag set each memory access will generate a syncronos exception)

```
	/* Mapping device memory*/
	mov 	x1, #DEVICE_BASE					// start mapping from device base address 
	ldr 	x2, =(VA_START + DEVICE_BASE)				// first virtual address
	ldr	x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)	// last virtual address
	create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4
```

Then device registers region is all mapped. This is done exactly in the same way as in the previous code sample, with the exception that we are now using different start and end addresses and different flags.

```
	mov	x30, x29						// restore return address
	ret
```

Finaly, the function restored link register and returns to the caller.

### Configuring page translation

Now page table are created and we are back to the `el1_entry` function. But there is still some work to be done, before we can switch on the MMU. Here is what happens.

```
	mov	x0, #VA_START			
	add	sp, x0, #LOW_MEMORY
```

We are updating init task stack pointer. Now it uses virtuall address, instead of physicall address. (Therefore it could be used only after MMU is on)

```
	adrp	x0, pg_dir				
	msr	ttbr1_el1, x0
```

`ttbr1_el1` is updated to point to the previously populated PGD table. 

```
	ldr	x0, =(TCR_VALUE)		
	msr	tcr_el1, x0
```

`tcr_el1` of Translation Control Register is responsible for configuring some general parameters of the MMU. (For example, here we configure that bosh kernel and user page tables should use 4 KB pages) 

```
	ldr	x0, =(MAIR_VALUE)
	msr	mair_el1, x0
```

We already discussed `mair` register in the "Configuring page attributes" section. Here we just set its value. 

```
	mov	x0, #SCTLR_MMU_ENABLED				
	msr	sctlr_el1, x0
```

This is the place were MMU is actually enabled. 

```
	ldr	x2, =kernel_main
	br 	x2
```

Now we can jump the the `kernel_main` function. An interesting question is why can't we just execute `br kernel_main` instruction? Indeed, we can't. Before the MMU was enabled we have been working with physical memory, the kernel is loaded at a physical offset 0 - this means that current program counter is very clse to 0. Switching on the MMU doesn't update the programm counter. If we now execute `br kernel_main` instruction, this instruction will use offset relative to the current programm counter and jumps to the place were `kernel_main` would have been if we don't turn on the MMU. `ldr	x2, =kernel_main` on the other hand loads `x2` with the absolute address of the `kernel_main` function. Because of the fact that we  set image base address to `0xffff000000000000` in the linker script, absolute address of the `kernel_main` function will be calculate as offset of thie function from the beggining of the image plus `0xffff000000000000` - wich is exactly what we need.

### Allocating user processes

If you work with a real OS you would probably expet it to be cappable of reading your program from the file system and executing it. This is different for the RPi OS - it doesn't have file system support yet. We were not bothered by this fact in the previous lessons, because user processes shared the same address space with the kernel. Now things has changed and each process should have its own address space, so we need to figure out how to store the user program so we can later load it into the newely created process. The trick that I end up implementing is to store the user program in a separate section of the kernel image. Here is the relevant section of the linker script that is responsible for doing this.

```
	. = ALIGN(0x00001000);
	user_begin = .;
	.text.user : { build/user* (.text) }
	.rodata.user : { build/user* (.rodata) }
	.data.user : { build/user* (.data) }
	.bss.user : { build/user* (.bss) }
	user_end = .;
```

I made a convenction that user level source code should be defined in the files with `user` prefix. The linker script then can isolate all user related code in a contignious region and define `user_begin` and `user_end` variables, wich mark the beggining and end of this region. In this way we can simply copy everything between `user_begin` and `user_end` to the newely allocated process address space, thus simulating loading a user program. This is simple enough and works well for our curent purpose, but after we implement file system support and will be able to load ELF files we will get rid of this hack.

Right now there are 2 files that are compiled in the user region.

* [user_sys.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/user_sys.S) This file contains definitions of the syscall wraper functions. The RPi OS still suports the same syscalls as in the previous lesson, with the exception that now instead of `clone` syscall we are going to use `fork` syscall. The difference is that `fork` copies process virtual memory, and that is something we want to try doing.
* [user.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/user.c) User program source code. Almost the same as we've used in the previous lesson.
### Creating first user process

As it was the case in the previous lesson, [move_to_user_mode](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/fork.c#L44) function is responsible for creating the first user process. We call this function from a kernel thread. Here is how we do this.

```
void kernel_process(){
	printf("Kernel process started. EL %d\r\n", get_el());
	unsigned long begin = (unsigned long)&user_begin;
	unsigned long end = (unsigned long)&user_end;
	unsigned long process = (unsigned long)&user_process;
	int err = move_to_user_mode(begin, end - begin, process - begin);
	if (err < 0){
		printf("Error while moving process to user mode\n\r");
	} 
}
```

Now we need 3 arguments to call `move_to_user_mode`: a pointer to the beggining of the user code area, size of the area and offset of the startup function inside it. This information is calculated based on the previously discussed `user_begin`  and `user_end` variables.

`move_to_user_mode` function is listed below.

```
int move_to_user_mode(unsigned long start, unsigned long size, unsigned long pc)
{
	struct pt_regs *regs = task_pt_regs(current);
	regs->pstate = PSR_MODE_EL0t;
	regs->pc = pc;
	regs->sp = 2 *  PAGE_SIZE;  
	unsigned long code_page = allocate_user_page(current, 0);
	if (code_page == 0)	{
		return -1;
	}
	memcpy(start, code_page, size);
	set_pgd(current->mm.pgd);
	return 0;
}
```

Now let's try to inspect in details what is going on here.

```
	struct pt_regs *regs = task_pt_regs(current);
```

As it was the case in the previous lesson, we obtain a pointer to `pt_regs` area and set `pstate`, so that after `kernel_exit` we will end up in EL0.

```
	regs->pc = pc;
```

`pc` now points to the offset of the startup function in the user region. Next we are going to copy the whole user region to the new address space, starting from offset 0, so that offset in the user region will become actuall virtuall address.

```
	regs->sp = 2 *  PAGE_SIZE;  
```

We made a simple convention that our user program will not exceed 1 page in size. We allocate second page to the stack.

```
	unsigned long code_page = allocate_user_page(current, 0);
	if (code_page == 0)	{
		return -1;
	}
```

`allocate_user_page` reserves 1  memory page and maps it to the virtuall address, provided as a second argument. In the process of mapping it populates page tables, assotiated with the current process. We will investigate in details how this function works in later in this chapter.

```
	memcpy(start, code_page, size);
```

Next, we copy the whole user region to the page that we have just mapped. 

```
	set_pgd(current->mm.pgd);
```

Finally, we call [set_pgd](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/utils.S#L24), wich updates `ttbr0_el1` register and thus activate current process translation tables.

### TLB (Translation lookaside buffer)

If you take a look at the `set_pgd` function you will see that after it sets `ttbr0_el1` it also cleares TLB(https://en.wikipedia.org/wiki/Translation_lookaside_buffer) (Translation lookaside buffer). TLB is a cache that is designed specifically to store mapping between pysical and virtual pages. The first time an address of some virtual page is mapped into a physical address this mapping is stored in TLB. Next time we need to access the same page we no longer need to perform full page table walk. Therefore it makes perfect sense that we invalidate TLB after updating page tables - otherwise our cahnge will not be applied for the pages allready stored in the TLB.

Usually we try to avoid using all caches for simplicity, but without TLB any memory access would become extremelly inefficien, and I don't thing that it is even posible to sompletely disable TLB. Besides, TLB don't add any other complexity to the OS, inspite of the fact that we must clean it after switching `ttbr0_el1`.

### Mapping a virtual page

We have seen previously how [allocate_user_page](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L14) function is used - now it is time to see what is inside it.

```
unsigned long allocate_user_page(struct task_struct *task, unsigned long va) {
	unsigned long page = get_free_page();
	if (page == 0) {
		return 0;
	}
	map_page(task, va, page);
	return page + VA_START;
}
```

This function allocates a new page, maps it to the provided virtuall address and returns a pointerto the page. When we say "a pointer" now we need to distingush between 3 things: a pointer to the physical page, a pointer inside kernel address space and a pinter inside user address space - all this 3 different pointers can lead to the same mocation in memory. In our case `page` variable is a physical pointer and return value is a pointer inside kernel address space. This pointer can be easily calculated because we lineary mapp the whole physical memory starting at `VA_START` virtuall address. We also don't need to worry about allocating new kernel page table because all of the memory is allready mapped in the `boot.S`. But user mapping is still required to be created and this happens in the [map_page](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L62) function wich we will explore next.

```
void map_page(struct task_struct *task, unsigned long va, unsigned long page){
	unsigned long pgd;
	if (!task->mm.pgd) {
		task->mm.pgd = get_free_page();
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = task->mm.pgd;
	}
	pgd = task->mm.pgd;
	int new_table;
	unsigned long pud = map_table((unsigned long *)(pgd + VA_START), PGD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = pud;
	}
	unsigned long pmd = map_table((unsigned long *)(pud + VA_START) , PUD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = pmd;
	}
	unsigned long pte = map_table((unsigned long *)(pmd + VA_START), PMD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = pte;
	}
	map_table_entry((unsigned long *)(pte + VA_START), va, page);
	struct user_page p = {page, va};
	task->mm.user_pages[task->mm.user_pages_count++] = p;
}
```

`map_page` in some way duplicates what we've been doing in the `__create_page_tables` function: it allocates and populates a page table hierarchy. There are 3 important difference however: now we are doing this in C, instead of assembler, `map_page` maps a single page, instead of the whole memory and `map_page` uses normal page mapping, instead of section mapping.

There are 2 important functions involved in the process:  [map_table](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L47) and [map_table_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L40). 

`map_table` is listed below.

```
unsigned long map_table(unsigned long *table, unsigned long shift, unsigned long va, int* new_table) {
	unsigned long index = va >> shift;
	index = index & (PTRS_PER_TABLE - 1);
	if (!table[index]){
		*new_table = 1;
		unsigned long next_level_table = get_free_page();
		unsigned long entry = next_level_table | MM_TYPE_PAGE_TABLE;
		table[index] = entry;
		return next_level_table;
	} else {
		*new_table = 0;
	}
	return table[index] & PAGE_MASK;
}
```

This function has the following arguments.

* `table` This is a pointer to the parent page table. This page table is assumed to be already allocated, but might be empty.
* `shift` This argument is used to extract table indes from the virtuall address.
* `new_table` This is output parameter. It is set to 1 if a new child table has been allocated and left 0 otherwise.

You can think of this function as an analog of the `create_table_entry` macro. It extracts table index from the virtuall address and prepares a descriptor in the parent table that points to the child table. Unlike `create_table_entry` macro we don't assume that child table sould be adjasted into memory with the parent table - instead we rely on `get_free_table` function to return whatever page is available. It also might be the case that child table was already allocated (This might happen if child page table covers the region were another page has been allocated previously) In this case we set `new_table` to 1 and read child page table address from the parent table.

`map_page` calls `map_table` 3 times: once for PGD, PUD and PMD. The last call allocates PTE and sets a descriptor in the PMD. Next, `map_table_entry` is called. You can see this function below.

```
void map_table_entry(unsigned long *pte, unsigned long va, unsigned long pa) {
	unsigned long index = va >> PAGE_SHIFT;
	index = index & (PTRS_PER_TABLE - 1);
	unsigned long entry = pa | MMU_PTE_FLAGS; 
	pte[index] = entry;
}
```

`map_table_entry` extracts PTE intex from the virtuall address and then prepares and sets PTE descriptor. It it similar to what we've been doing in the `create_block_map` macro. 

That's it about user page tables allocation. But `map_page` is responsible for one more important role: it keeps track of the pages that has been allocated during virtuall address mapping. All such pages are stored in the [kernel_pages](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/sched.h#L53) array. We need this array to be able to cleanup allocated pages after a task exits. There is also [user_pages](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/sched.h#L51), wich is also populated by the `map_page` function. This array store information about the corespondence between prcess virtual page any physical pages. We need this information in order to be able to copy process virtuall memory during `fork` (More on this later)

### Forking a process

Before we move forward let me summarize were we are so far: we've seen how first user process is created, its page tables populated, source code copied to the proper location and stack initialized. After all of this preparation the process is ready ro run. The code that is executed inside user process is listed below.

```
void loop(char* str)
{
	char buf[2] = {""};
	while (1){
		for (int i = 0; i < 5; i++){
			buf[0] = str[i];
			call_sys_write(buf);
			user_delay(1000000);
		}
	}
}

void user_process() 
{
	call_sys_write("User process\n\r");
	int pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("Error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		loop("abcde");
	} else {
		loop("12345");
	}
}
``` 

The code itself is very simple. The only tricky part is the semantic of the `fork` system call. Unlike `clone`, when doing `fork` we don't need to provide the function that need to be executed in a new process. Also the [fork wrapper function](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/user_sys.S#L26) is much easier then `clone` one. All of this is posible because of the fact that `fork` make a full copy of the process virtuall address space, so the fork wrapper function return twice: one time in the original process and one time in the new one. At this point we have two identical processes, with identical stacks and `pc` positions.The only difference is the return value of the fork syscall: it returns child PID in the parent process and 0 in the child process. Starting from this process both processes begin completely independent live and can modify their stacks and write different things using same addresses in memory - all of this without affecting one another.

Now let's see how `fork` system call is implemented. [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/fork.c#L7) function does most of the job.

```
int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg)
{
	preempt_disable();
	struct task_struct *p;

	unsigned long page = allocate_kernel_page();
	p = (struct task_struct *) page;
	struct pt_regs *childregs = task_pt_regs(p);

	if (!p)
		return -1;

	if (clone_flags & PF_KTHREAD) {
		p->cpu_context.x19 = fn;
		p->cpu_context.x20 = arg;
	} else {
		struct pt_regs * cur_regs = task_pt_regs(current);
		*cur_regs = *childregs;
		childregs->regs[0] = 0;
		copy_virt_memory(p);
	}
	p->flags = clone_flags;
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1; //disable preemtion untill schedule_tail

	p->cpu_context.pc = (unsigned long)ret_from_fork;
	p->cpu_context.sp = (unsigned long)childregs;
	int pid = nr_tasks++;
	task[pid] = p;	

	preempt_enable();
	return pid;
}
```

This function looks almost exactly the same as in the previous lesson with one exception: when copying user processes, now, instead of modifying new process stack pointer and program counter, we instead call [copy_virt_memory](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L87). `copy_virt_memory` looks like this.

```
int copy_virt_memory(struct task_struct *dst) {
	struct task_struct* src = current;
	for (int i = 0; i < src->mm.user_pages_count; i++) {
		unsigned long kernel_va = allocate_user_page(dst, src->mm.user_pages[i].virt_addr);
		if( kernel_va == 0) {
			return -1;
		}
		memcpy(src->mm.user_pages[i].virt_addr, kernel_va, PAGE_SIZE);
	}
	return 0;
}
```

It iterates over `user_pages` array, wich contains all pages, allocated by the current process. Note, that in `user_pages` array we store only pages that are actually available to the process and contains its source code or data; we don't include here page table page, wich are stored in `kernel_pages` array. Next for each page we allocate another empty page and copy the original page content there. We also map the new page using the same virtuall address, that is used by the original page. This is how we get exact copy of the original process address space.

All other details of the forking procedure works exactly in the same way, as they have been in the previous lesson.

### Allocating new pages on demand

If you go back and take a look on the `move_to_user_mode` function, you may notice that we only map a single page, starting at offset 0. But we also assume that second page will be used as a stack. Why don't we map second page as well? If you thing it is a bug, it is not - it is a feature! Stack page, as well as any other page that a process need to access will be mapped as soon as it will be requested for the first time. Now we are going to explore the inner-workings of this mechanizm.

When a process tries to access some address wich belongs to the page that is not yet mapped a syncronos exception is generated. This is the second type of syncronos exception that we are going to support (the first type is an exception generated by the `svs` instruction wich is a sysstemcall) Syncronos exception handler now looks like the following.

```
el0_sync:
	kernel_entry 0
	mrs	x25, esr_el1				// read the syndrome register
	lsr	x24, x25, #ESR_ELx_EC_SHIFT		// exception class
	cmp	x24, #ESR_ELx_EC_SVC64			// SVC in 64-bit state
	b.eq	el0_svc
	cmp	x24, #ESR_ELx_EC_DABT_LOW		// data abort in EL0
	b.eq	el0_da
	handle_invalid_entry 0, SYNC_ERROR
```

Here we use `esr_el1` register to determine exception type. If it is a page fault exception (or, wich is the same, data access exception) `el0_da` function is called.

```
el0_da:
	bl	enable_irq
	mrs	x0, far_el1
	mrs	x1, esr_el1			
	bl	do_mem_abort
	cmp x0, 0
	b.eq 1f
	handle_invalid_entry 0, DATA_ABORT_ERROR
1:
	bl disable_irq				
	kernel_exit 0
```

`el0_da` redirects the main work to the [do_mem_abort](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L101) function. This fanction takes 2 arguments
1. The memory address wich we tried to access. This address is taken from `far_el1`  register (Fault address register)
1. The content of the `esr_el1` (Exception syndrome register)

`do_mem_abort` is listed below.

```
int do_mem_abort(unsigned long addr, unsigned long esr) {
	unsigned long dfs = (esr & 0b111111);
	if ((dfs & 0b111100) == 0b100) {
		unsigned long page = get_free_page();
		if (page == 0) {
			return -1;
		}
		map_page(current, addr & PAGE_MASK, page);
		ind++;
		if (ind > 2){
			return -1;
		}
		return 0;
	}
	return -1;
}
```

In order to understand this function you need to know a little bit about the specifics of that `esr_el1` register. Bits [32:26] of this register are called "Exception Class". We check those bits in the `el0_sync` handler to determine whether is is a syscall, or a data abort exception or potentially something else. Exception class determines the meaning of bits [24:0] - those bits are usually used to provide additional information about the exception. The meaning of [24:0] bits in case of the data abort exception is described on the page 1525 of the `AArch64-Reference-Manual`. In general data abort exception can happen in many different scenarios (it could be a permission fault, or address size fault or a lot of other things) We are only interested in a translation fault wich happens when some of the page table for the curent virtuall address are not initialized.
So in the first 2 lines of the `do_mem_abort` function we check whether current exception is actually a translation fault. If yes we allocate a new page and map it to the requested virtuall address. All of this happens completely transpared for the user program - it doesn't notice that some of the memory accesses were interrupted and new page tables were allocated in the meantime.

### Conclusion

This was a long and difficult chapter, but I hope it was usefull as well. Virtual memory is really one of the most fundamental pices of any operating system and I am glad we've passe thought this chapter and, hopefully, start to understand how it works at the lowest level. With the introduction of virtual memory now we have full process isolation, but the RPi OS is still far from completion. It still don't support file systems, drivers, signals and interrupt waitlists, networking and a lot of other usefull concepts, and we will continue to uncover them in the upcomming lessons.
