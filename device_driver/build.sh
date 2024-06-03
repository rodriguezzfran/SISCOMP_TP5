#!/bin/bash
 
make clean
make
sudo rmmod CDD_GPIO_BUTTON 2>/dev/null
sudo insmod CDD.ko
echo "CDD_GPIO_BUTTON driver is loaded"
MAJOR=$(awk '$2=="CDD_GPIO_BUTTON" {print $1}' /proc/devices)
echo $MAJOR
sudo mknod /dev/CDD_GPIO_BUTTON c $MAJOR 0
sudo chmod 666 /dev/CDD_GPIO_BUTTON