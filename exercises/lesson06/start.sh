#!/bin/bash

echo "###"
echo "### Use C-a h for help"
echo "###"
echo ""

qemu-system-aarch64 -machine raspi3 -nographic -kernel kernel8.img -serial null -serial mon:stdio
