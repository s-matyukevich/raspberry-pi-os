#! /usr/bin/env bash

qemu-system-aarch64 -m 128 \
 -M raspi3 \
 -cpu cortex-a53 \
 -kernel kernel8.img \
 -nographic \
 -serial null \
 -chardev stdio,id=uart1 \
 -serial chardev:uart1 \
 -monitor none
