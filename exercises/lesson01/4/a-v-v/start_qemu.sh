#!/usr/bin/bash
#https://github.com/s-matyukevich/raspberry-pi-os/issues/8#issuecomment-522339426
qemu-system-aarch64 -m 128 -M raspi3 -cpu cortex-a53 -kernel kernel8.img \
                    -nographic -serial null -chardev stdio,id=tty0 \
                    -serial chardev:tty0 -monitor none
