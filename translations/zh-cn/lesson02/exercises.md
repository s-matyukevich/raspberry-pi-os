## 2.3: Exercises

1. 与其直接从EL3跳到EL1, 不如尝试先到达EL2, 然后再切换到EL1. 
2. 在学习本课程时, 我遇到的一个问题是, 如果使用`FP/SIMD`寄存器, 那么EL3的一切运行正常,  但是一旦进入EL1, 打印功能就会停止工作. 这就是为什么我添加 [-mgeneral-regs-only](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/Makefile#L3) 编译器选项的参数. 现在, 我希望您删除此参数并重现此行为.  接下来, 您可以使用 `objdump` 工具来查看gcc在没有 `-mgeneral-regs-only` 标志的情况下如何充分利用 `FP/SIMD` 寄存器.  最后, 我希望您使用 `cpacr_el1` 来允许使用 `FP/SIMD` 寄存器. 
3. 改编第02课以在`qemu`上运行.  校验 [这个](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue 供参考. 

##### 上一页

2.2 [处理器初始化：Linux](../../docs/lesson02/linux.md)

##### Next Page

3.1 [中断处理：RPi OS](../../docs/lesson03/rpi-os.md)
