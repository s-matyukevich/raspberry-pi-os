## 先决条件

### 1. [Raspberry Pi 3 Model B](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/)

较旧版本的Raspberry Pi无法在本教程中使用，因为所有课程均设计为使用支持ARMv8架构的64位处理器，并且该处理器仅在`Raspberry Pi 3`中可用。

> 较新的版本，包括[Raspberry Pi 3型号B +]（https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/）应该可以正常工作，尽管尚未对其进行测试。

### 2. [USB to TTL serial cable](https://www.amazon.com/s/ref=nb_sb_noss_2?url=search-alias%3Daps&field-keywords=usb+to+ttl+serial+cable&rh=i%3Aaps%2Ck%3Ausb+to+ttl+serial+cable) 

获得串行电缆后，需要测试连接。如果您从未做过此事，我建议您遵循[本指南]（https://cdn-learn.adafruit.com/downloads/pdf/adafruits-raspberry-pi-lesson-5-using-a-console-cable.pdf）详细描述了通过串行电缆连接Raspberry PI的过程。

该指南还介绍了如何使用串行电缆为Raspberry Pi供电。 RPi OS在这种设置下运行良好，但是，在这种情况下，您需要在插入电缆后立即运行终端仿真器。检查[this]（https://github.com/s-matyukevich/raspberry-pi-os/issues/2）
 问题的细节..

### 3. [SD card](https://www.raspberrypi.org/documentation/installation/sd-cards.md) with installed [Raspbian OS](https://www.raspberrypi.org/downloads/raspbian/)

我们首先需要Raspbian来测试USB至TTL电缆的连接。另一个原因是，安装后它将以正确的方式格式化SD卡。

### 4. Docker

严格来说，Docker不是必需的依赖项。使用Docker构建课程的源代码非常方便，特别是对于Mac和Windows用户而言。

每节课都有`build.sh`脚本（或Windows用户的`build.bat`）此脚本使用Docker来构建课程的源代码。

可以在[官方docker网站]（https://docs.docker.com/engine/installation/）上找到有关如何为您的平台安装docker的说明。

>  交叉编译

如果出于某些原因要避免使用Docker，则可以安装[make utility]（http://www.math.tau.ac.il/~danha/courses/software1/make-intro.html）以及`aarch64-linux-gnu`工具链。
如果您使用的是Ubuntu，则只需安装`gcc-aarch64-linux-gnu`和`build-essential`软件包。
