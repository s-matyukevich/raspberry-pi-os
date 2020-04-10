## 3.5: Exercises

1. 使用本地计时器而不是系统计时器来生成处理器中断. 有关详细信息, 请参见[这篇](https://github.com/s-matyukevich/raspberry-pi-os/issues/70) 问题. 
2. 处理`MiniUART`中断. 用什么都不做的循环代替`kernel_main`函数中的最终循环. 设置`MiniUART`器件以在用户键入新字符后立即生成中断. 实现一个中断处理程序, 该处理程序负责在屏幕上打印每个新到达的字符. 
3. 改编第03课以在`qemu`上运行. 检查[这个](https://github.com/s-matyukevich/raspberry-pi-os/issues/8)问题以供参考. 

##### 上一页

3.4 [中断处理：计时器](../../docs/lesson03/linux/timer.md)

##### 下一页

4.1 [进程调度程序：RPi OS调度程序](../../docs/lesson04/rpi-os.md)
