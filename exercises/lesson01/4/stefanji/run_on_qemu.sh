#! /usr/bin/env bash

path='../../../../src/lesson01/'

# build lesson01
echo "cd $path"
cd $path
./build.sh $1

# run on qemu
echo "Will running on qemu, ctl-c to exit qemu process"
qemu-system-aarch64 -m 128 \
 -M raspi3 \
 -cpu cortex-a53 \
 -kernel kernel8.img \
 -nographic \
 -serial null \
 -chardev stdio,id=uart1 \
 -serial chardev:uart1 \
 -monitor none
