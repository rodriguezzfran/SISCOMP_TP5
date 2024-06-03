#!/bin/bash

# Compilar y cargar el módulo
make
sudo insmod CDD.ko

# Obtener el número mayor asignado automáticamente
MAJOR=$(awk '$2=="CDD_GPIO_BUTTON" {print $1}' /proc/devices)

# Verificar que se obtuvo el número mayor
if [ -z "$MAJOR" ]; then
    echo "Error: no se pudo obtener el número mayor."
    exit 1
fi

# Crear el archivo de dispositivo
sudo mknod /dev/CDD_GPIO_BUTTON c $MAJOR 0
sudo chmod 666 /dev/CDD_GPIO_BUTTON

echo "Dispositivo creado con número mayor $MAJOR"