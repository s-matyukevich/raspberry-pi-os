# How to install qemu to emulate ARM v.8
I followed this site:
https://raspberrypi.stackexchange.com/questions/34733/how-to-do-qemu-emulation-for-bare-metal-raspberry-pi-images/85135#85135

# How to run
qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial stdio

