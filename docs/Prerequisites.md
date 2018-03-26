## Prerequisites

### 1. [Raspberry Pi 3 model b](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) 

Older versions of Raspberry Pi are not going to work with this tutorial because all lessons are designed to use a 64-bit processor that supports ARMv8 architecture and such processor is only available in Raspberry Pi 3. Newer versions, including [Raspberry Pi 3 model b +](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/) should work fine, though I haven't tested it yet.

### 2. [USB to TTL serial cable](https://www.amazon.com/s/ref=nb_sb_noss_2?url=search-alias%3Daps&field-keywords=usb+to+ttl+serial+cable&rh=i%3Aaps%2Ck%3Ausb+to+ttl+serial+cable) 

After you get serial cable, you need to test your connection. If you never did this before I recommend you to follow [this guide](https://cdn-learn.adafruit.com/downloads/pdf/adafruits-raspberry-pi-lesson-5-using-a-console-cable.pdf) It describes the process of connecting your Raspberry PI via serial cable in great details. One thing that I can recommend you to do is to use your serial cable to power your Raspberry Pi. How to do this is described in the link above.

### 3. [SD card](https://www.raspberrypi.org/documentation/installation/sd-cards.md) with installed [Raspbian OS](https://www.raspberrypi.org/downloads/raspbian/)

We need Raspbian to test USB to TTL cable connectivity initially. Another reason is that after installation it leaves the SD card formatted in the way we need.

### 4. Docker

Strictly speaking, Docker is not a required dependency. It is just convenient to use Docker to build source code of the lessons, especially for Mac and Windows users. Each lesson has `build.sh` script (or `build.bat` for windows users) This script uses Docker to build source code of the lesson. Instructions how to install docker for your platform can be found on the [official docker website](https://docs.docker.com/engine/installation/)  

If for some reasons you want to avoid using Docker, you can install [make utility](http://www.math.tau.ac.il/~danha/courses/software1/make-intro.html) as well as  `aarch64-linux-gnu` toolchain. If you are using Ubuntu you just need to install `gcc-aarch64-linux-gnu` and `build-essential` packages.

