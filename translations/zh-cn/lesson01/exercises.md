## 1.5: 练习

练习是可选的，尽管我强烈建议您稍微尝试一下源代码. 如果您能够完成任何练习 - 请与他人分享您的源代码. 有关详细信息，请参见[contribution guide](../Contributions.md).

1. 引入一个常数`baud_rate`, 使用该常数计算必要的Mini UART寄存器值. 确保程序可以使用115200以外的波特率工作。
2. 更改操作系统代码以使用UART设备代替Mini UART. 采用 `BCM2837 ARM Peripherals` 手册弄清楚如何访问UART寄存器以及如何配置GPIO引脚。
3. 尝试使用所有4个处理器内核. 操作系统应打印 `Hello, from processor <processor index>` 对于不同核心. 不要忘记为每个内核设置单独的堆栈，并确保Mini UART仅初始化一次. 您可以结合使用全局变量和 `delay` 同步功能.
4. 改编第01课以在qemu上运行. 查看 [这个](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue以供参考.

##### 上一页

1.4 [Kernel 初始化: Linux启动顺序](../../../translations/zh-cn/lesson01/linux/kernel-startup.md)

##### 下一页

2.1 [处理器初始化: RPi OS](../../../translations/zh-cn/lesson02/rpi-os.md)
