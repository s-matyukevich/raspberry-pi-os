## 1.2：Linux项目结构

这是我们第一次谈论Linux. 这个想法是首先完成编写我们自己的内核的一小步, 然后看一下相同的东西在Linux中的工作方式. 到目前为止, 我们仍然做得很少：仅实施了我们的第一个裸机`hello world` 程序, 我们仍然能够发现RPi OS和Linux之间的某些相似之处. 

现在我们将探索其中的一些. 

### 项目结构

每当您开始研究任何大型软件项目时, 都需要快速浏览一下项目结构. 这非常重要, 因为它使您能够了解由哪些模块组成项目以及什么是高级体系结构. 

让我们尝试探索Linux内核的项目结构. 

首先, 您需要克隆Linux系统信息库. 

```
git clone https://github.com/torvalds/linux.git
cd Linux
git checkout v4.14
```

我们使用的是`v4.14`版本, 因为这是撰写本文时的最新版本. 使用此特定版本将对Linux源代码进行所有引用. 

接下来, 让我们看一下我们可以在Linux系统信息库中找到的文件夹. 我们不会研究所有, 只会研究我认为最重要的那些. 

* [arch](https://github.com/torvalds/linux/tree/v4.14/arch) 此文件夹包含子文件夹, 每个子文件夹用于特定的处理器体系结构. 通常, 我们将使用 [arm64](https://github.com/torvalds/linux/tree/v4.14/arch/arm64) - 这是与 ARM.v8 处理器兼容的版本. 
* [init](https://github.com/torvalds/linux/tree/v4.14/init) 内核始终由体系结构特定的代码引导. 但是随后就会将执行传递给 [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) 函数, 该函数负责常见的内核初始化同时该函数也是与体系结构无关的内核起点. 在`start`文件夹中定义了`start_kernel`函数以及一些其他初始化函数. 
* [kernel](https://github.com/torvalds/linux/tree/v4.14/kernel) 这是Linux内核的核心. 几乎所有主要的内核子系统都在此实现. 
* [mm](https://github.com/torvalds/linux/tree/v4.14/mm) 在此处定义了与内存管理(`Memory mangement`)相关的所有数据结构和方法. 
* [drivers](https://github.com/torvalds/linux/tree/v4.14/drivers) 这是Linux内核中最大的文件夹. 它包含所有设备驱动程序的实现. 
* [fs](https://github.com/torvalds/linux/tree/v4.14/fs) 您可以在此处查找不同的文件系统实现. 

这种解释是非常高层次的, 但是到目前为止已经足够了. 在下一章中, 我们将尝试更详细地研究Linux构建系统. 

#### 上一页

1.1 [内核初始化：引入RPi OS或裸机 “Hello, world！”](../../../docs/lesson01/rpi-os.md)

#### 下一页

1.3 [内核初始化：内核构建系统](../linux/build-system.md)
