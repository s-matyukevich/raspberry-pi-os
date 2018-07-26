#!/bin/bash

mount -L rpiboot /mnt
cp "kernel8.img" /mnt
umount /mnt
