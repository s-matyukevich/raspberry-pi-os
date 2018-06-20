#!/bin/bash

./build.sh

if [ $1 == "pi" ]; then
  cp -f config.txt kernel8.img /Volumes/boot/ && diskutil umount /Volumes/boot
elif [ $1 == "qemu" ]; then
  qemu-system-aarch64 -M raspi3 -smp 4 -nographic -kernel kernel-qemu.img
fi

