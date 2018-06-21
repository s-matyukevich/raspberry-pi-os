#!/bin/bash

qemu-system-aarch64 -machine raspi3 -serial null -serial mon:stdio -nographic -kernel kernel7.img
