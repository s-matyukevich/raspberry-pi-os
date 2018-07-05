#!/bin/bash

if [ $# -ne 1 ]; then
    echo "usage: $0 kernel"
    exit 1
fi

mount -L rpiboot /mnt
cp "$1" /mnt
umount /mnt
