## 2.1: 处理器初始化

在本课程中, 我们将与`ARM`处理器更加紧密地合作. 它具有一些可由操作系统使用的基本功能. 第一个这样的功能称为 "Eception Levels"(异常级别). 

### 特权级别

每个支持`ARM.v8`体系结构的`ARM`处理器都有`4`个 异常级别. 您可以将异常级别(简称 `EL`)视为处理器的执行模式, 在不同的执行模式下只有一部分操作和寄存器中可用. 最低的*EL*是`0`. 当处理器在该级别上运行时, 它通常仅使用通用寄存器(`X0-X30`)和栈指针寄存器(`SP`). `EL0` 还允许使用 `STR` 和 `LDR` 命令从内存中加载和存储数据, 以及用户程序通常使用的其他一些指令. 

操作系统为了实现进程隔离, 会去负责异常级别的处理. 用户进程不应能够访问其他进程的数据. 为了实现这种行为, 操作系统始终在`EL0`上运行每个用户进程. 在此异常级别上运行时, 进程只能使用它自己的虚拟内存, 并且不能访问任何会更改虚拟内存设置的指令. 因此, 为了做到进程隔离, 操作系统需要为每个进程准备独立的虚拟内存映射, 而且在将处理器执行到用户进程之前, 需要将处理器转入`EL0` 级别. 

操作系统本身通常在 `EL1` 上运行. 在此异常级别运行时, 处理器可以访问允许配置虚拟内存设置的寄存器以及某些系统寄存器. Raspberry Pi OS 也将使用 `EL1`. 

我们不会大量使用异常级别2和3, 但是我只想简要地描述它们, 以便您了解为什么需要它们. 

`EL2` 用于我们使用虚拟机监控程序的场景. 在这种情况下, 主机操作系统在`EL2`上运行, 而访客操作系统只能使用 `EL1`. 这允许主机`OS`以隔离用户进程类似的方式来隔离访客`OS`. 

`EL3`用于从 ARM `安全世界` 到 `不安全世界` 的过渡. 存在这种抽象是为了给运行在两个不同的 `世界` 中的软件提供完全的硬件隔离. 来自 `不安全世界` 的应用程序绝不能访问或修改属于 `安全世界` 的信息(指令和数据), 并且这种限制是在硬件级别上强制执行的. 

### 调试内核

我要做的下一件事是弄清楚我们当前正在使用的异常级别. 但是当我尝试执行此操作时, 我意识到内核只能在屏幕上打印一些常量字符串, 但是我需要的是类似 [printf](https://en.wikipedia.org/wiki/Printf_format_string) 的函数. 使用 `printf`, 我可以轻松显示不同寄存器和变量的值. 这样的功能对于内核开发是必不可少的, 因为您没有任何其他调试器可用, `printf` 就成为您确定程序内部正在发生什么的唯一手段. 

对于 RPi OS, 我决定不重新发明轮子, 而是使用现有的一种 [printf](http://www.sparetimelabs.com/tinyprintf/tinyprintf.php) 的实现. 该函数主要由字符串操作组成, 从内核开发人员的角度来看不是很有趣. 我使用的这个实现很小, 并且没有外部依赖关系, 因此可以轻松地将其集成到内核中. 我唯一要做的就是定义可以将单个字符发送到屏幕的`putc`函数. 此函数在[mini_uart.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/mini_uart.c#L59) 定义, 它只是使用了已经存在的 `uart_send` 函数. 同样, 我们需要初始化 `printf` 库并指定 `putc` 函数的位置. 这是在 [kernel.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L8) 中完成的. 

### 查找当前的异常级别

现在, 当我们具备 `printf` 函数时, 我们可以完成我们的原始任务：确定操作系统在哪个异常级别启动. 一个可以回答这个问题的小函数在[utils.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/utils.S#L1) 中定义. 

```
.globl get_el
get_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret
```

在这里, 我们使用 `mrs` 指令将 `CurrentEL` 系统寄存器中的值读入 `x0` 寄存器中. 然后, 我们将这个值向右移2位(我们需要这样做, 因为`CurrentEL`寄存器中的前2位是保留位, 并且始终为`0`), 最后在寄存器`x0`中, 我们有一个整数, 表示当前的异常水平. 现在剩下的唯一事情就是显示此值, 例如[kerner.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L10). 

```
    int el = get_el();
    printf("Exception level: %d \r\n", el);
```

如果您重现此实验, 在屏幕上应该能看到 `Exception level：3`. 

### 更改当前的异常级别

在 `ARM` 体系结构中, 如果没有已经在更高级别上运行的软件的参与, 程序就无法增加自己的异常级别. 这很有道理, 如果没有这个限制, 则任何程序都可以更改当前的 `EL` , 然后去访问其他程序的数据. 

所以只有当发生了异常时, 才能更改当前的 EL. 如果程序执行某些非法指令(例如, 尝试访问不存在的内存地址、试图除以`0`), 则可能会发生这种情况. 应用程序也可以执行 `svc` 指令来故意产生异常. 硬件生成的中断也被视为特殊类型的异常. 每当生成异常时, 都会触发以下操作(在描述中, 我假设异常是在 EL`n` 处处理的, 而`n`可能是`1`、`2`或`3`). 

1. 当前指令的地址保存在 `ELR_ELn` 寄存器中(`Exception link register`)
2. 当前处理器状态存储在 `SPSR_ELn` 寄存器中(`Saved Program Status Register`)
3. 异常处理程序将运行并执行所需的任何工作
4. 异常处理程序调用`eret`指令. 该指令从 `SPSR_ELn` 恢复处理器状态, 并从存储在 `ELR_ELn` 寄存器中的地址开始恢复执行. 

在实践中, 该过程要复杂一些, 因为异常处理程序还需要存储所有通用寄存器的状态, 然后将其还原回去, 但是我们将在下一课中详细讨论该过程. 现在, 我们只需要大致了解该过程, 并记住`ELR_ELm`和`SPSR_ELn`寄存器的含义即可. 

要知道的重要一点是, 异常处理程序没有义务返回到异常所源自的相同位置.  **`ELR_ELm` 和 `SPSR_ELn` 都是可写的**, 如果需要, 异常处理程序可以对其进行修改. 当我们尝试在代码中从 `EL3` 切换到 `EL1` 时, 我们将利用这种技术来发挥优势. 

### 切换到EL1

严格来说, 我们的操作系统不是必须切换到`EL1`, 但是`EL1`对我们来说是很自然的选择, 因为该级别具有执行所有常见 `OS` 任务的正确权限集. 看看切换异常级别是如何工作的, 这也是一个有趣的练习. 让我们看一下[boot.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L17). 

```
master:
    ldr    x0, =SCTLR_VALUE_MMU_DISABLED
    msr    sctlr_el1, x0        

    ldr    x0, =HCR_VALUE
    msr    hcr_el2, x0

    ldr    x0, =SCR_VALUE
    msr    scr_el3, x0

    ldr    x0, =SPSR_VALUE
    msr    spsr_el3, x0

    adr    x0, el1_entry        
    msr    elr_el3, x0

    eret                
```

如您所见, 该代码主要由配置一些系统寄存器组成.  现在我们将逐一检查这些寄存器. 为此, 我们首先需要下载 [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile). 本文档包含 `ARM.v8` 体系结构的详细规范. 

#### SCTLR_EL1, 系统控制寄存器 (EL1), Page 2654 of AArch64-Reference-Manual.

```
    ldr    x0, =SCTLR_VALUE_MMU_DISABLED
    msr    sctlr_el1, x0        
```

在这里, 我们设置 `sctlr_el1` 系统寄存器的值.  `sctlr_el1` 负责在 EL1 上运行时配置处理器的不同参数. 例如, 它控制是否启用缓存以及对我们最重要的是是否打开 `MMU`(Memory Mapping Unit: 内存映射单元). 可以从所有高于或等于 `EL1` 的异常级别访问 `sctlr_el1` 寄存器(您也可以从 `_el1` 后缀中推断出这一点). 

`SCTLR_VALUE_MMU_DISABLED` 是一个常量, 定义在 [sysregs.h](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L16) 中. 

该值的各个位的定义如下：

* `#define SCTLR_RESERVED(3 << 28)|(3 << 22)|(1 << 20)|(1 << 11)` sctlr_el1 寄存器描述中的某些位被标记为 `RES1`(Reserve). 这些保留位是供将来使用的, 应将其初始化为`1`. 
* `#define SCTLR_EE_LITTLE_ENDIAN (0 << 25)` 异常的[字节序](https://en.wikipedia.org/wiki/Endianness). 该字段控制在 EL1 处进行显式数据访问的顺序. 我们将配置处理器仅在 `little-endian` 下工作. 
* `#define SCTLR_EOE_LITTLE_ENDIAN (0 << 24)` 与上一字段类似, 但此字段控制 `EL0` 而不是`EL1`处的显式数据访问的字节序. 
* `#define SCTLR_I_CACHE_DISABLED (0 << 12)` 禁用指令缓存. 为了简单起见, 我们将禁用所有缓存. 您可以在[此处](https://stackoverflow.com/questions/22394750/what-is-meant-by-data-cache-and-instruction-cache)找到有关数据和指令高速缓存的更多信息. 
* `#define SCTLR_D_CACHE_DISABLED (0 << 2)` 禁用数据缓存. 
* `#define SCTLR_MMU_DISABLED (0 << 0)` 禁用MMU. 在第6课之前, 必须禁用 MMU, 在第6课中, 我们将准备页表并开始使用虚拟内存. 

#### HCR_EL2, 系统管理程序配置寄存器 (EL2), Page 2487 of AArch64-Reference-Manual. 

```
    ldr    x0, =HCR_VALUE
    msr    hcr_el2, x0
```

我们不会实施我们自己的[hypervisor](https://en.wikipedia.org/wiki/Hypervisor). 直到我们需要使用该寄存器, 因为在其他设置中, 它控制着EL1的执行状态. 执行状态必须是`AArch64`而不是`AArch32`. 此配置在[sysregs.h](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L22).

#### SCR_EL3, 安全配置寄存器 (EL3), Page 2648 of AArch64-Reference-Manual.

```
    ldr    x0, =SCR_VALUE
    msr    scr_el3, x0
```

该寄存器负责配置安全设置. 例如, 它控制所有较低级别是在 `安全` 状态还是 `非安全` 状态下执行. 它还控制 `EL2` 的执行状态. 我们设置EL2将在`AArch64`处执行可参考在[这](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L26). 所有较低的异常级别将是 `不安全的`. 

#### SPSR_EL3, 储存程序状态寄存器 (EL3), Page 389 of AArch64-Reference-Manual.

```
    ldr    x0, =SPSR_VALUE
    msr    spsr_el3, x0
```

该寄存器应该已经为您所熟悉-在讨论更改异常级别的过程时我们提到了它. `spsr_el3` 包含处理器状态, 在我们执行 `eret` 指令后将恢复该状态. 

值得说几句话来说明什么是处理器状态. 处理器状态包括以下信息：

* **Condition Flags** 这些标志位包含了之前执行的操作的信息：结果是负数(N标志), 零(A标志), 无符号溢出(C标志)还是有符号溢出(V标志). 这些标志的值可以在条件分支指令中使用. 例如, 仅当上一次比较操作的结果等于0时, `b.eq`指令才会跳转到所提供的标签. 处理器通过测试Z标志是否设置为1来进行检查. 

* **Interrupt disable bits** 这些位允许启用/禁用不同类型的中断. 

* **其他信息** 处理异常后, 完全恢复处理器执行状态所需的一些其他信息. 

通常, 当 EL3 发生异常时, 会自动保存`spsr_el3`. 但是该寄存器是可写的, 因此我们利用这一事实并手动准备处理器的状态. 在[sysregs.h](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L35) 准备了`SPSR_VALUE`, 并初始化了以下域：

* `#define SPSR_MASK_ALL (7 << 6)` 将`EL`更改为`EL1`后, 所有类型的中断都将被屏蔽(或禁用)
* `#define SPSR_EL1h (5 << 0)` 在`EL1`, 我们可以使用自己专用的栈指针, 也可以使用`EL0`栈指针.  `EL1h`模式意味着我们正在使用 `EL1` 的专用栈指针.  

#### ELR_EL3, 异常链接寄存器 (EL3), Page 351 of AArch64-Reference-Manual.

```
    adr    x0, el1_entry        
    msr    elr_el3, x0

    eret                
```

`elr_el3` 保存地址, 在执行 `eret` 指令后, 我们将返回该地址. 在这里, 我们将此地址设置为 `el1_entry` 标签的位置. 

### 结论

差不多了：当我们输入 `el1_entry` 函数时, 执行应该已经处于`EL1`模式. 尝试下吧！

##### 上一页

1.5 [内核初始化：练习](./exercises.md)

##### 下一页

2.2 [Linux 处理器初始化](./linux.md)
