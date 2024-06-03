#!/bin/bash
 
make clean
make
sudo rmmod CDD.ko 2>/dev/null
echo "CDD_GPIO_BUTTON driver is unloaded"
sudo sudo dmesg -C
echo "message is cleared"
sudo insmod CDD.ko
echo "CDD_GPIO_BUTTON driver is loaded"
MAJOR=$(awk '$2=="CDD_GPIO_BUTTON" {print $1}' /proc/devices)
if [ -z $MAJOR ]; then
    echo "Device number is not found"
    exit
fi
echo $MAJOR
sudo mknod /dev/CDD_GPIO_BUTTON c $MAJOR 0
sudo chmod 666 /dev/CDD_GPIO_BUTTON