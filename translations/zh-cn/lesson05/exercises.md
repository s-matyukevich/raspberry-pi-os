## 5.3: 练习题

1. 在用户模式下执行任务时，请尝试访问某些系统寄存器。在这种情况下，请确保生成同步异常。处理此异常时，请使用`esr_el1`寄存器将其与系统调用区分开。
1. 实施可用于设置当前任务优先级的新系统调用。演示在任务运行时如何动态应用优先级更改。
1. 改编第05课以在qemu上运行。 校验 [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) 供参考。

##### 上一页

5.2 [用户进程和系统调用：Linux](../../docs/lesson05/linux.md)

##### 下一页

6.1 [虚拟内存管理：RPi OS](../../docs/lesson06/rpi-os.md)
