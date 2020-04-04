## 6.1: 虚拟内存管理

RPi OS现在可以运行和调度用户进程, 但是它们之间的隔离还不完全-所有进程和内核本身共享相同的内存. 这允许任何进程轻松访问他人的数据, 甚至内核数据. 即使我们假设所有进程都不是恶意程序, 也存在另一个缺点：在分配内存之前, 每个进程都需要知道哪些内存区域已被占用-这会使进程的内存分配更加复杂. 

### Translation process

在本课程中, 我们将通过引入虚拟内存来解决上述所有问题. 虚拟内存为每个进程提供了一个抽象, 使它认为它占用了所有可用内存. 每次进程需要访问某个内存位置时, 它都会使用虚拟地址, 该地址被转换为物理地址. 翻译过程对于该过程是完全透明的, 并且由特殊设备`MMU`(内存映射单元)执行.  `MMU`使用转换表以便将虚拟地址转换为物理地址. 下图描述了翻译过程. 

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

以下事实对于全面理解此图和内存转换过程至关重要. 

* 进程的内存始终以页分配. 一个页面是一个大小为4KB的内存区域(ARM处理器支持更大的页面, 但是4KB是最常见的情况, 我们将讨论仅限于此页面大小). 
* 页表具有层次结构. 在任何表中的项目包含了下表中的层次结构中的地址. 
* 表层次结构中有4个级别：PGD(页面全局目录), PUD(页面上层目录), PMD(页面中间目录), PTE(页面表条目).  PTE是层次结构中的最后一个表, 它指向物理内存中的实际页面. 
* 内存转换过程从查找PGD(页面全局目录)表的地址开始. 该表的地址存储在 `ttbr0_el1` 寄存器中. 每个进程都有其所有页表(包括PGD)的副本, 因此每个进程必须保留其`PGD`地址. 在上下文切换期间, 下一个进程的PGD地址被加载到 `ttbr0_el1` 寄存器中. 
* 接下来, MMU使用PGD指针和虚拟地址来计算相应的物理地址. 在64个可用位中, 所有虚拟地址仅使用48个. 进行翻译时, MMU将地址分为4部分：
  * [39-47] 位包含PGD表中的索引.  MMU使用此索引来查找PUD的位置. 
  * [30-38] 位包含PUD表中的索引.  MMU使用此索引来查找PMD的位置. 
  * [21-29] 位包含PMD表中的索引.  MMU使用此索引查找PTE的位置. 
  * [12-20] 位包含PTE表中的索引.  MMU使用此索引在物理内存中查找页面. 
  * [0-11] 位包含物理页中的偏移量.  MMU使用此偏移量来确定先前找到的页面中与原始虚拟地址相对应的确切位置. 

现在, 让我们做一个小练习并计算页面表的大小. 从上图中, 我们知道页表中的索引占用9位(对于所有页表级别都是如此). 这意味着每个页表都包含 `2^9=512` 个项目. 页表中的每个项目都是层次结构中下一个页表的地址, 如果是PTE, 则是物理页的地址. 当我们使用64位处理器时, 每个地址的大小必须为64位或8个字节. 综合所有这些, 我们可以计算出页表的大小必须为 `512 *8 = 4096` 字节或4 KB. 但这恰好是一页的大小！这可能会给您一个直觉, 为什么 `MMU` 设计人员选择了这样的数字. 

### 剖面图

在开始查看源代码之前, 我还要讨论另一件事：节映射. 有时是需要映射的连续物理内存大的部分. 在这种情况下, 我们可以直接映射 `2 MB` 的块, 而不是 `4 KB` 的页面, 这称为节. 这样可以消除1级翻译. 在这种情况下, 翻译图如下所示. 

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

如您所见, 这里的区别是现在`PMD`包含一个指向物理部分的指针. 此外, 偏移量占用的是`21`位而不是`12`位(这是因为我们需要`21`位才能对`2MB`范围进行编码)

### 页面描述符格式

您可能会问, MMU如何知道PMD项是指向PTE还是物理的 `2 MB` 部分？为了回答这个问题, 我们需要仔细查看页表项的结构. 现在, 我可以承认, 声称页面表中的项始终包含下一个页面表或物理页面的地址时, 我并不是很准确：每个此类项还包含一些其他信息. 页表中的一项称为 `描述符` . 描述具有特殊格式, 如下所述. 

```
                           Descriptor format
`+------------------------------------------------------------------------------------------+
 | Upper attributes | Address (bits 47:12) | Lower attributes | Block/table bit | Valid bit |
 +------------------------------------------------------------------------------------------+
 63                 47                     11                 2                 1           0
```

这里要理解的关键是, 每个描述符始终指向页面对齐的内容(层次结构中的物理页面, 节或下一页表). 这意味着存储在描述符中的地址的最后`12`位将始终为`0`. 这也意味着MUU可以使用这些位来存储更有用的内容 - 正是它的作用. 现在让我解释一下描述符中所有位的含义. 

* **Bit 0** 对于所有有效的描述符, 必须将该位设置为1. 如果MMU在转换过程中遇到无效描述符, 则会生成同步异常. 然后, 内核应处理该异常, 分配新页面并准备正确的描述符(稍后将详细介绍其工作原理)
* **Bit 1** 该位指示当前描述符是指向层次结构中的下一个页面表(我们称该描述符为 `表描述符` )还是指向物理页面或部分(此类描述符称为`块描述符`). 
* **Bits [11:2]** 这些位被表描述符忽略. 对于块描述符, 它们包含一些控制属性, 例如, 映射的页面是否可缓存, 可执行等. 
* **Bits [47:12]** 这是描述符指向的地址的存储位置. 正如我之前提到的, 仅需要存储地址的位[47:12], 因为所有其他位始终为0. 
* **Bits [63:48]** 另一组属性. 

### 配置页面属性

正如我在上一节中提到的那样, 每个块描述符都包含一组控制各种虚拟页面参数的属性. 但是, 对于我们的讨论而言最重要的属性未在描述符中直接配置. 相反, ARM处理器实现了一个技巧, 使他们可以在描述符属性部分中节省一些空间. 

ARM.v8体系结构引入了 `mair_el1` 寄存器. 该寄存器由8个部分组成, 每个部分为8位长. 每个此类部分都配置了一组公共属性. 然后, 描述符仅指定 `mair` 部分的索引, 而不是直接指定所有属性. 这允许在描述符中仅使用3位来引用 `mair` 部分.  `air` 部分中每个位的含义在 `AArch64-Reference-Manual` 的第2609页中进行了描述. 在RPi OS中, 我们仅使用一些可用的属性选项.  [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/arm/mmu.h#L11)  是为 `air` 寄存器准备值的代码. 

```cpp
/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]
 *            n    MAIR
 *   DEVICE_nGnRnE    000    00000000
 *   NORMAL_NC        001    01000100
 */
#define MT_DEVICE_nGnRnE         0x0
#define MT_NORMAL_NC            0x1
#define MT_DEVICE_nGnRnE_FLAGS        0x00
#define MT_NORMAL_NC_FLAGS          0x44
#define MAIR_VALUE            (MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC))
```

在这里, 我们仅使用`mair`寄存器中8个可用插槽中的2个. 第一个对应于设备内存, 第二个对应于普通不可缓存内存.  `MT_DEVICE_nGnRnE`和`MT_NORMAL_NC`是我们将要在块描述符使用索引, `MT_DEVICE_nGnRnE_FLAGS`和`MT_NORMAL_NC_FLAGS`是我们正在存储在第一2个时隙的`mair_el1`寄存器值. 

### 内核与用户虚拟内存

MMU打开后, 每个内存访问必须使用虚拟内存而不是物理内存. 这一事实的结果是, 内核本身必须准备使用虚拟内存并维护自己的页表集. 一种可能的解决方案是, 每当我们从用户模式切换到内核模式时, 都要重新加载pgd寄存器. 问题是切换pgd是非常昂贵的操作, 因为它要求所有缓存无效. 考虑到我们需要从用户模式切换到内核模式的频率, 此解决方案将使缓存完全无用, 因此该解决方案从未在OS开发中使用. 操作系统正在做的是将地址空间分为两部分：用户空间和内核空间.  32位体系结构通常为用户程序分配前3 GB的地址空间, 为内核保留后1 GB的地址空间. 在这方面, 64位体系结构由于其巨大的地址空间而更加受青睐. 甚至更多：ARM.v8体系结构具有本机功能, 可用于轻松实现用户/内核地址拆分. 

有2个寄存器可以保存PGD的地址：`ttbr0_el1` 和 `ttbr1_el1`. 您可能还记得, 我们在64个可用地址中只使用了48位, 因此高16位可用于区分 `ttbr0`和 `ttbr1` 转换过程. 如果高16位都等于0, 则使用存储在 `ttbr0_el1` 中的 `PGD` 地址, 如果地址以 `0xffff` 开头(前16位均等于1), 则选择存储在 `ttbr1_el1` 中的PGD地址.  . 该体系结构还确保运行在EL0上的进程永远不会访问以 `0xffff` 开头的虚拟地址, 而不会产生同步异常. 从该描述中, 您可以轻松推断出指向内核PGD的指针存储在`ttbr1_el1`中, 并在内核的整个生命周期中都保存在其中, 而`ttbr0_el1`用于存储当前用户进程PGD. 

这种方法的一个含义是, 所有绝对内核地址都必须以 `0xffff` 开头. 在我们处理此问题的过程中, RPi OS源代码中有2个地方.  在 [linker script](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/linker.ld#L3) 里面  我们将图片的基址指定为 `0xffff000000000000`. 这将使编译器认为我们的映像将被加载到 `0xffff000000000000` 地址, 因此, 每当需要生成绝对地址时, 它将正确处理. (链接器脚本还有一些其他更改, 但是稍后我们将讨论它们. )

我们还要对绝对内核基地址进行硬编码, 这还有一个地方： 在 [header](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/peripherals/base.h#L7) 里面我们定义设备基址的位置. 现在, 我们将访问从 `0xffff00003F000000` 开始的所有设备内存, 当然, 为了使其正常工作, 我们需要首先映射内核需要访问的所有内存. 在下一节中, 我们将详细探讨创建此映射的代码. 

### 初始化内核页表

创建内核页表的过程是我们在引导过程的早期就需要处理的事情.  它开始于 [boot.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L42) 文件.  在我们切换到 `EL1` 并清除 `BSS` 之后 [__create_page_tables](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L92) 函数被调用. 让我们逐行检查它. 

```
__create_page_tables:
    mov    x29, x30                        // save return address
```

首先, 该函数保存 `x30`(链接寄存器). 当我们从 `__create_page_tables` 调用其他函数时, `x30` 将被覆盖. 通常将 `x30` 保存在堆栈中, 但是, 由于我们知道我们将不使用递归, 并且在执行 `__create_page_tables` 期间没有其他人会使用 `x29`, 因此这种保存链接寄存器的简单方法也可以正常工作. 

```
    adrp    x0, pg_dir
    mov    x1, #PG_DIR_SIZE
    bl     memzero
```

接下来, 我们清除初始页表区域. 这里要了解的重要一件事是该区域的位置以及如何知道其大小？ 初始页表区域在 [linker script](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/linker.ld#L20) - 这意味着我们正在内核映像本身中为该区域分配斑点. 计算该区域的大小有些麻烦. 首先, 我们需要了解初始内核页表的结构. 我们知道所有映射都在1 GB区域内(这是RPi内存的大小). 一个PGD描述符可以覆盖 `2^39=512 GB` , 而一个PUD描述符可以覆盖 `2^30=1 GB` 的连续虚拟映射区域.  (这些值是根据虚拟地址中的PGD和PUD索引位置计算的. )这意味着我们只需要一个PGD和一个PUD即可映射整个RPi存储器, 甚至更多-PGD和PUD都将包含一个描述符. 如果我们只有一个PUD条目, 那么还必须有一个PMD表, 该条目将指向该表.  (单个PMD条目占2 MB, 一个PMD中有512个项目, 因此, 整个PMD表总共覆盖由单个PUD描述符覆盖的相同1 GB内存. )

接下来, 我们知道我们需要映射1 GB的内存区域, 这是2 MB的倍数 - 因此我们可以使用节映射. 这意味着我们根本不需要PTE. 因此, 总共需要3个页面：一个用于PGD, PUD和PMD, 这恰好是初始页面表区域的大小. 

现在, 我们要走出 `__create_page_tables` 函数之外, 并看一下2个基本宏: [create_table_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L68) 和 [create_block_map](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L77).

`create_table_entry`负责分配一个新的页表(在我们的例子中是PGD或PUD). 源代码在下面列出. 

```
    .macro    create_table_entry, tbl, virt, shift, tmp1, tmp2
    lsr    \tmp1, \virt, #\shift
    and    \tmp1, \tmp1, #PTRS_PER_TABLE - 1            // table index
    add    \tmp2, \tbl, #PAGE_SIZE
    orr    \tmp2, \tmp2, #MM_TYPE_PAGE_TABLE
    str    \tmp2, [\tbl, \tmp1, lsl #3]
    add    \tbl, \tbl, #PAGE_SIZE                    // next level table page
    .endm
```

该宏接受以下参数. 

* `tbl` - 指向必须分配新表的内存区域的指针. 
* `virt` - 当前正在映射的虚拟地址. 
* `shift` - 我们需要将其应用于虚拟地址以提取当前表索引.  (如果是PGD, 则为39；如果是PUD, 则为30)
* `tmp1`, `tmp2` - 临时寄存器. 

这个宏非常重要, 因此我们将花一些时间来理解它. 

```
    lsr    \tmp1, \virt, #\shift
    and    \tmp1, \tmp1, #PTRS_PER_TABLE - 1            // table index
```

宏的前两行负责从虚拟地址中提取表索引. 我们首先应用右移将所有内容剥离到索引的右侧, 然后使用`和`操作将所有内容剥离到左侧. 

```
    add    \tmp2, \tbl, #PAGE_SIZE
```

然后计算下一页表的地址. 在这里, 我们使用的约定是, 所有初始页表都位于一个连续的内存区域中. 我们仅假设层次结构中的下一个页面表将与当前页面表相邻. 

```
    orr    \tmp2, \tmp2, #MM_TYPE_PAGE_TABLE
```

接下来, 将指向层次结构中下一页表的指针转换为表描述符.  (描述符必须将2个低位设置为 `1`)

```
    str    \tmp2, [\tbl, \tmp1, lsl #3]
```

然后将描述符存储在当前页表中. 我们使用先前计算的索引在表中找到正确的位置. 

```
    add    \tbl, \tbl, #PAGE_SIZE                    // next level table page
```

最后, 我们更改 `tbl` 参数以指向层次结构中的下一页表. 这很方便, 因为现在我们可以为层次结构中的下一个表再调用一次 `create_table_entry`, 而无需对`tbl`参数进行任何调整. 这正是我们在 [create_pgd_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/boot.S#L63) 宏, 这只是分配PGD和PUD的包装器. 

下一个重要的宏是`create_block_map`, 您可能会猜到该宏负责填充PMD表的条目. 看起来如下. 

```
    .macro    create_block_map, tbl, phys, start, end, flags, tmp1
    lsr    \start, \start, #SECTION_SHIFT
    and    \start, \start, #PTRS_PER_TABLE - 1            // table index
    lsr    \end, \end, #SECTION_SHIFT
    and    \end, \end, #PTRS_PER_TABLE - 1                // table end index
    lsr    \phys, \phys, #SECTION_SHIFT
    mov    \tmp1, #\flags
    orr    \phys, \tmp1, \phys, lsl #SECTION_SHIFT            // table entry
9999:    str    \phys, [\tbl, \start, lsl #3]                // store the entry
    add    \start, \start, #1                    // next entry
    add    \phys, \phys, #SECTION_SIZE                // next block
    cmp    \start, \end
    b.ls    9999b
    .endm
```

这里的参数有些不同. 

* `tbl` - 指向PMD表的指针. 
* `phys` - 要映射的物理区域的开始. 
* `start` - 要映射的第一部分的虚拟地址. 
* `end` - 要映射的最后一部分的虚拟地址. 
* `flags` - 这些标志需要复制到块描述符的较低属性中. 
* `tmp1` - 临时注册. 

现在, 让我们检查源代码. 

```
    lsr    \start, \start, #SECTION_SHIFT
    and    \start, \start, #PTRS_PER_TABLE - 1            // table index
```

这两行从 `start` 虚拟地址中提取表索引. 完全以与之前在 `create_table_entry` 宏中相同的方式完成. 

```
    lsr    \end, \end, #SECTION_SHIFT
    and    \end, \end, #PTRS_PER_TABLE - 1                // table end index
```

对 `结束` 地址重复相同的操作. 现在, `开始`和`结束`都不再包含虚拟地址, 而是PMD表中的索引, 对应于原始地址. 

```
    lsr    \phys, \phys, #SECTION_SHIFT
    mov    \tmp1, #\flags
    orr    \phys, \tmp1, \phys, lsl #SECTION_SHIFT            // table entry
```

接下来, 准备块描述符并将其存储在 `tmp1` 变量中. 为了准备描述符, 首先将 `phys`参数右移, 然后使用`orr`指令将其与`flags`参数合并. 如果您想知道为什么我们必须来回移动地址-答案是, 这将清除`phys`地址中的前`21`位并使我们的宏通用, 从而使其可用于任何地址, 而不仅是第一个地址的部分. 

```
9999:    str    \phys, [\tbl, \start, lsl #3]                // store the entry
    add    \start, \start, #1                    // next entry
    add    \phys, \phys, #SECTION_SIZE                // next block
    cmp    \start, \end
    b.ls    9999b
```

函数的最后一部分在循环内执行. 在这里, 我们首先将当前描述符存储在PMD表中的正确索引处. 接下来, 我们将当前索引增加1并更新描述符以指向下一部分. 我们重复相同的过程, 直到当前索引等于最后一个索引. 

现在, 当您了解`create_table_entry`和`create_block_map`宏如何工作时, 将很容易理解`__create_page_tables`函数的其余部分. 

```
    adrp    x0, pg_dir
    mov    x1, #VA_START
    create_pgd_entry x0, x1, x2, x3
```

在这里, 我们创建PGD和PUD. 我们将它们配置为从以下位置开始映射 [VA_START](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/mm.h#L6) 虚拟地址.  即PMD  -因为`create_table_entry`宏的语义, 后`create_pgd_entry`完成`x0`将包含层次结构中的下一个表的地址. 

```
    /* Mapping kernel and init stack*/
    mov     x1, xzr                            // start mapping from physical offset 0
    mov     x2, #VA_START                        // first virtual address
    ldr    x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)        // last virtual address
    create_block_map x0, x1, x2, x3, MMU_FLAGS, x4
```

接下来, 我们创建整个内存的虚拟映射, 不包括设备寄存器区域.  我们用 [MMU_FLAGS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/arm/mmu.h#L24) 常量作为 `flags` 参数-将所有要映射的部分标记为普通的不可缓存内存.  (注意, `MM_ACCESS`标志也被指定为 `MMU_FLAGS`常量的一部分. 没有该标志, 每个内存访问将生成一个同步异常. )

```
    /* Mapping device memory*/
    mov     x1, #DEVICE_BASE                    // start mapping from device base address
    ldr     x2, =(VA_START + DEVICE_BASE)                // first virtual address
    ldr    x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)    // last virtual address
    create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4
```

然后映射设备寄存器区域. 除了使用不同的开始和结束地址以及不同的标志外, 这与上一个代码示例完全相同. 

```
    mov    x30, x29                        // restore return address
    ret
```

最后, 该函数恢复了链接寄存器并返回到调用方. 

### 配置页面翻译

现在创建页面表, 回到 `el1_entry` 函数. 但是, 在打开MMU之前, 仍有一些工作要做. 这是发生了什么. 

```
    mov    x0, #VA_START
    add    sp, x0, #LOW_MEMORY
```

我们正在更新初始化任务堆栈指针. 现在, 它使用虚拟地址, 而不是物理地址.  (因此, 它可以仅是MMU上后使用. )

```
    adrp    x0, pg_dir
    msr    ttbr1_el1, x0
```

`ttbr1_el1` 已更新为指向先前填充的PGD表. 

```
    ldr    x0, =(TCR_VALUE)
    msr    tcr_el1, x0
```

转换控制寄存器的`tcr_el1`负责配置MMU的一些常规参数.  (例如, 在这里, 我们配置内核和用户页面表都应使用4 KB页面. )

```
    ldr    x0, =(MAIR_VALUE)
    msr    mair_el1, x0
```

我们已经在`配置页面属性`部分讨论了 `mair` 寄存器. 在这里, 我们只是设置它的值. 

```
    ldr    x2, =kernel_main

    mov    x0, #SCTLR_MMU_ENABLED
    msr    sctlr_el1, x0

    br     x2
```

`msr sctlr_el1, x0`是实际启用MMU的行. 现在我们可以跳转到`kernel_main`函数. 一个有趣的问题是为什么我们不能只执行`br kernel_main`指令？确实, 我们做不到. 在启用MMU之前, 我们一直在处理物理内存, 内核以物理偏移量0加载-这意味着当前程序计数器非常接近0. 打开MMU不会更新程序计数器. 如果现在执行 `br kernel_main` 指令, 则该指令将使用相对于当前程序计数器的偏移量, 并跳转到如果不打开MMU将会是 `kernel_main` 的位置. 另一方面, `ldr x2, = kernel_main` 将使用 `kernel_main` 函数的绝对地址加载 `x2` . 由于我们在链接描述文件中将图片基址设置为 `0xffff000000000000` , 因此 `kernel_main` 函数的绝对地址将被计算为距图片开头的偏移量加上 `0xffff000000000000` , 这正是我们需要. 您需要了解的另一件事是为什么在打开MMU之前必须执行`ldr x2, = kernel_main`指令. 原因是 `ldr` 也使用 `pc` 相对偏移, 因此, 如果我们尝试在MMU开启之后但在跳转到图像基地址之前执行该指令, 则该指令将产生页面错误. 

### 分配用户进程

如果您使用的是真正的操作系统, 则可能希望它能够从文件系统读取程序并执行它. 对于RPi OS, 这是不同的-它还没有文件系统支持. 在上一课中, 我们并不为这个事实所困扰, 因为用户进程与内核共享相同的地址空间. 现在情况已经改变, 每个进程都应该有自己的地址空间, 因此我们需要弄清楚如何存储用户程序, 以便以后将其加载到新创建的进程中. 我最终实现的技巧是将用户程序存储在内核映像的单独部分中. 这是链接器脚本的相关部分, 负责执行此操作. 

```
    . = ALIGN(0x00001000);
    user_begin = .;
    .text.user : { build/user* (.text) }
    .rodata.user : { build/user* (.rodata) }
    .data.user : { build/user* (.data) }
    .bss.user : { build/user* (.bss) }
    user_end = .;
```

我约定, 应在带有`user`前缀的文件中定义用户级源代码. 然后, 链接程序脚本可以在连续区域中隔离所有与用户相关的代码, 并定义 `user_begin` 和 `user_end`变量, 以标记该区域的开始和结束. 这样, 我们可以简单地将 `user_begin` 和 `user_end` 之间的所有内容复制到新分配的进程地址空间, 从而模拟加载用户程序. 这足够简单, 并且可以很好地用于我们当前的目的, 但是在实现文件系统支持并能够加载ELF文件之后, 我们将摆脱这种黑客. 

现在, 在用户区域中编译了2个文件. 

* [user_sys.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/user_sys.S) 该文件包含`syscall`包装函数的定义.  RPi OS仍然支持与上一课相同的系统调用, 除了现在我们将使用 `fork` 系统调用代替 `clone` 系统调用. 区别在于`fork`复制进程虚拟内存, 而这正是我们要尝试做的事情. 
* [user.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/user.c) 用户程序源代码. 几乎与上一课中使用的相同. 

### 创建第一个用户进程

就像上一课一样,  [move_to_user_mode](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/fork.c#L44) 函数负责创建第一个用户进程. 我们从内核线程调用此函数. 这是我们的操作方式. 

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

现在我们需要3个参数来调用`move_to_user_mode`：指向用户代码区域开始处的指针, 该区域的大小以及其中的启动函数的偏移量. 该信息是根据先前讨论的`user_begin`和`user_end`变量计算的. 

下面列出了`move_to_user_mode`函数. 

```cpp
int move_to_user_mode(unsigned long start, unsigned long size, unsigned long pc)
{
    struct pt_regs *regs = task_pt_regs(current);
    regs->pstate = PSR_MODE_EL0t;
    regs->pc = pc;
    regs->sp = 2 *  PAGE_SIZE;
    unsigned long code_page = allocate_user_page(current, 0);
    if (code_page == 0)    {
        return -1;
    }
    memcpy(code_page, start, size);
    set_pgd(current->mm.pgd);
    return 0;
}
```

现在, 让我们尝试详细检查此处发生的情况. 

```
    struct pt_regs *regs = task_pt_regs(current);
```

与上一课一样, 我们获得了指向`pt_regs`区域的指针并设置了`pstate`, 因此在`kernel_exit`之后, 我们将以EL0结尾. 

```
    regs->pc = pc;
```

现在, `pc` 指向用户区域中启动功能的偏移量. 

```
    regs->sp = 2 *  PAGE_SIZE;
```

我们做了一个简单的约定, 即用户程序的大小不得超过1页. 我们将第二页分配到堆栈. 

```
    unsigned long code_page = allocate_user_page(current, 0);
    if (code_page == 0)    {
        return -1;
    }
```

`allocate_user_page`保留1个内存页并将其映射到虚拟地址(作为第二个参数提供). 在映射过程中, 它填充与当前过程关联的页表. 我们将在本章后面详细研究此功能的工作方式. 

```
    memcpy(code_page, start, size);
```

接下来, 我们将从偏移量0开始将整个用户区域复制到新的地址空间(在我们刚映射的页面中), 因此用户区域中的偏移量将成为起点的实际虚拟地址. 

```
    set_pgd(current->mm.pgd);
```

最后, 我们称 [set_pgd](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/utils.S#L24), 它会更新 `ttbr0_el1` 寄存器, 从而激活当前的进程转换表. 

### TLB (Translation lookaside buffer 翻译后备缓冲区)

如果看一下`set_pgd`函数, 您会发现它在设置了`ttbr0_el1`之后也会清除[TLB](https://en.wikipedia.org/wiki/Translation_lookaside_buffer) (Translation lookaside buffer). TLB是专门设计用于存储物理页面和虚拟页面之间的映射的缓存. 第一次将某个虚拟地址映射到物理地址时, 此映射存储在TLB中. 下次我们需要访问同一页面时, 我们不再需要执行整页表遍历. 因此, 在更新页表之后使TLB无效是完全合理的 - 否则, 我们的更改将不会应用于已经存储在TLB中的页面. 

通常, 为了简化起见, 我们尽量避免使用所有缓存, 但是如果没有TLB, 则任何内存访问都将变得效率极低, 而且我认为甚至不可能完全禁用TLB. 另外, 尽管我们必须在切换`ttbr0_el1` 之后清理它, 但TLB并没有给操作系统增加任何其他复杂性. 

### 映射虚拟页面

我们之前已经看过 [allocate_user_page](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L14) 函数被使用 - 现在该看里面的东西了. 

```cpp
unsigned long allocate_user_page(struct task_struct *task, unsigned long va) {
    unsigned long page = get_free_page();
    if (page == 0) {
        return 0;
    }
    map_page(task, va, page);
    return page + VA_START;
}
```

此函数分配一个新页面, 将其映射到提供的虚拟地址, 然后返回指向该页面的指针. 现在, 当我们说`指针`时, 我们需要区分三件事：指向物理页面的指针, 内核地址空间内的指针和用户地址空间内的指针-所有这三种不同的指针都可以导致内存中的相同位置. 在我们的例子中, `page`变量是物理指针, 返回值是内核地址空间内的指针. 这个指针很容易计算, 因为我们线性映射了整个物理内存, 从 `VA_START` 虚拟地址开始. 我们也不必担心分配新的内核页表, 因为所有的内存都已经映射到`boot.S`中.  仍然需要创建用户映射, 这种情况发生在 [map_page](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L62) 函数中, 接下来我们将探讨. 

```cpp
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

`map_page` 以某种方式复制了我们在 `__create_page_tables` 函数中所做的工作：它分配并填充了页面表层次结构. 但是, 有3个重要的区别：现在, 我们使用C而不是汇编器进行此操作.  `map_page` 映射单个页面而不是整个内存, 并使用普通页面映射而不是部分映射. 

该过程涉及2个重要功能:  [map_table](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L47) 和 [map_table_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L40). 

下面列出了 `map_table` . 


```cpp
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

此函数具有以下参数. 

* `table` 这是指向父页面表的指针. 假定此页表已经分配, ​​但是可能为空. 
* `shift` 此参数用于从提供的虚拟地址中提取表索引. 
* `va` 虚拟地址本身. 
* `new_table` 这是一个输出参数. 如果已分配了新的子表, 则设置为1, 否则设置为0. 

您可以将该函数视为`create_table_entry`宏的类似物. 它从虚拟地址中提取表索引, 并在父表中准备一个指向子表的描述符. 与`create_table_entry`宏不同, 我们不假定子表应该与父表在内存中相邻-相反, 我们依赖于`get_free_table`函数来返回可用的任何页面. 也可能是已经分配了子表(如果子页面表覆盖了先前已分配另一个页面的区域, 则可能会发生这种情况). 在这种情况下, 我们将 `new_table` 设置为0并从父表中读取子页表地址. 

`map_page`调用`map_table` 3次：一次用于`PGD`, `PUD`和`PMD`. 最后一个调用分配PTE并在PMD中设置描述符. 接下来, 调用 `map_table_entry`. 您可以在下面看到此功能. 

```cpp
void map_table_entry(unsigned long *pte, unsigned long va, unsigned long pa) {
    unsigned long index = va >> PAGE_SHIFT;
    index = index & (PTRS_PER_TABLE - 1);
    unsigned long entry = pa | MMU_PTE_FLAGS;
    pte[index] = entry;
}
```

`map_table_entry`从虚拟地址中提取`PTE`索引, 然后准备并设置`PTE`描述符. 这类似于我们在`create_block_map`宏中所做的事情. 

就是关于用户页表分配的, 但是`map_page`负责另一个重要的角色：它跟踪在虚拟地址映射过程中分配的页面. 所有此类页面均存储在 [kernel_pages](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/sched.h#L53) 数组. 我们需要此数组才能在任务退出后清理分配的页面.  也有 [user_pages](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/include/sched.h#L51) 数组, 也由` map_page` 函数填充. 该数组存储有关进程虚拟页面与任何物理页面之间的对应关系的信息. 我们需要此信息, 以便能够在`fork`期间复制进程虚拟内存(稍后会有更多信息). 

### Forking a process

在继续前进之前, 让我总结一下到目前为止的情况：我们已经了解了如何创建第一个用户进程, 如何填充其页表, 将源代码复制到正确的位置并进行了堆栈初始化. 完成所有准备工作后, 该过程即可运行. 下面列出了在用户进程内部执行的代码. 

```cpp
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

代码本身非常简单. 唯一棘手的部分是 `fork` 系统调用的语义. 与`clone`不同, 在执行`fork`时, 我们不需要提供需要在新进程中执行的功能.  而且, [fork wrapper function Fork包装功能](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/user_sys.S#L26) 比 `clone` 容易得多.  所有这些都是可能的, 因为`fork`会完整复制进程虚拟地址空间, 因此fork包装函数会返回两次：在原始进程中一次, 在新进程中一次. 此时, 我们有两个相同的过程, 具有相同的堆栈和 `pc` 位置. 唯一的区别是`fork` `syscall`的返回值：它在父进程中返回子PID, 在子进程中返回0. 从这一点开始, 两个进程都开始完全独立的生活, 并且可以使用内存中的相同地址修改其堆栈并写入不同的内容-所有这些都不会相互影响. 


现在让我们看看如何实现fork系统调用.  [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/fork.c#L7) 函数做大部分的工作. 

```cpp
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
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        copy_virt_memory(p);
    }
    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;
    int pid = nr_tasks++;
    task[pid] = p;

    preempt_enable();
    return pid;
}
```

此功能看起来与上一课几乎完全相同, 但有一个例外：现在, 在复制用户进程时, 无需修改新的进程堆栈指针和程序计数器,  我们调用 [copy_virt_memory](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L87). `copy_virt_memory` 看起来像这样.

```cpp
int copy_virt_memory(struct task_struct *dst) {
    struct task_struct* src = current;
    for (int i = 0; i < src->mm.user_pages_count; i++) {
        unsigned long kernel_va = allocate_user_page(dst, src->mm.user_pages[i].virt_addr);
        if( kernel_va == 0) {
            return -1;
        }
        memcpy(kernel_va, src->mm.user_pages[i].virt_addr, PAGE_SIZE);
    }
    return 0;
}
```

遍历`user_pages`数组, 该数组包含当前进程分配的所有页面. 注意, 在 `user_pages` 数组中, 我们仅存储该过程实际可用的页面, 并包含其源代码或数据；我们这里不包括存储在`kernel_pages`数组中的页表页. 接下来, 对于每个页面, 我们分配另一个空页面, 然后在其中复制原始页面内容. 我们还将使用与原始页面相同的虚拟地址来映射新页面. 这就是我们获得原始进程地址空间的精确副本的方式. 

分支过程的所有其他细节的工作方式与上一课中所述的完全相同. 

### 按需分配新页面

如果回头看一下`move_to_user_mode`函数, 您可能会注意到我们只映射了一个从偏移量0开始的页面. 但是我们也假设第二页面将用作堆栈. 我们为什么还不映射第二页？如果您认为这是一个错误, 那不是-这是一个功能！第一次请求时, 将立即映射堆栈页面以及流程需要访问的任何其他页面. 现在, 我们将探索此机制的内部工作原理. 

当进程尝试访问属于尚未映射的页面的某个地址时, 将生成一个同步异常. 这是我们将要支持的第二种同步异常(第一类是由svc指令生成的异常, 它是系统调用). 现在, 同步异常处理程序如下所示. 

```
el0_sync:
    kernel_entry 0
    mrs    x25, esr_el1                // read the syndrome register
    lsr    x24, x25, #ESR_ELx_EC_SHIFT        // exception class
    cmp    x24, #ESR_ELx_EC_SVC64            // SVC in 64-bit state
    b.eq    el0_svc
    cmp    x24, #ESR_ELx_EC_DABT_LOW        // data abort in EL0
    b.eq    el0_da
    handle_invalid_entry 0, SYNC_ERROR
```

在这里, 我们使用`esr_el1`寄存器来确定异常类型. 如果是页面错误异常(或相同的数据访问异常), 则调用 `el0_da` 函数. 

```
el0_da:
    bl    enable_irq
    mrs    x0, far_el1
    mrs    x1, esr_el1
    bl    do_mem_abort
    cmp x0, 0
    b.eq 1f
    handle_invalid_entry 0, DATA_ABORT_ERROR
1:
    bl disable_irq
    kernel_exit 0
```

`el0_da`将主要工作重定向到 [do_mem_abort](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson06/src/mm.c#L101) 函数. 该函数有两个参数
1. 我们试图访问的内存地址. 该地址取自 `far_el1` 寄存器(故障地址寄存器)
1. `esr_el1`(异常综合征寄存器)的内容

下面列出了`do_mem_abort`. 

```cpp
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

为了理解这个功能, 您需要对`esr_el1`寄存器的细节有所了解. 该寄存器的位[32:26]被称为​​ `异常类`. 我们检查 `el0_sync` 处理程序中的那些位, 以确定它是系统调用, 数据异常中止还是其他可能的原因. 异常类确定位的含义[24：0] - 的那些位通常用来提供有关异常的附加信息. 在 `AArch64-Reference-Manual` 的第2460页中描述了发生数据异常中止时[24：0]位的含义. 通常, 数据中止异常可能会在许多不同的情况下发生(可能是权限错误, 地址大小错误或许多其他情况). 我们仅对转换错误感兴趣, 该错误在当前虚拟地址的某些页表未初始化时发生. 
因此, 在 `do_mem_abort` 函数的前两行中, 我们检查当前异常是否实际上是翻译错误. 如果是的, 我们分配了新的一页, 并将其映射到所请求的虚拟地址. 所有这些对于用户程序来说都是完全透明的-它没有注意到某些内存访问被中断, 同时还分配了新的页表. 

### 结论

这是一个漫长而艰难的章节, 但我希望它也有用. 虚拟内存确实是所有操作系统中最基本的部分之一, 我很高兴我们阅读了本章, 并希望开始了解它在最低级别上的工作方式. 随着虚拟内存的引入, 我们现在已经实现了完全的进程隔离, 但是RPi OS仍远未完成. 它仍然不支持文件系统, 驱动程序, 信号和中断等待列表, 网络以及许多其他有用的概念, 我们将在即将开始的课程中继续发现它们. 

##### 上一页

5.3 [用户流程和系统调用：练习](../../docs/lesson05/exercises.md)

##### 下一页

6.2 虚拟内存管理: Linux (进行中)  
6.3 跳到 [虚拟内存管理：练习](../../docs/lesson06/exercises.md)
