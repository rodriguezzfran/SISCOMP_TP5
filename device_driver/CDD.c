#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define GPIO_PIN "17"  // Pin GPIO que vamos a usar

int main(void) {
    int fd, value;
    char buf[64];
    ssize_t nbytes;

    // Exportar el pin GPIO
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo /sys/class/gpio/export");
        return -1;
    }
    write(fd, GPIO_PIN, strlen(GPIO_PIN));
    close(fd);

    // Configurar el pin GPIO como entrada
    sprintf(buf, "/sys/class/gpio/gpio%s/direction", GPIO_PIN);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo direction");
        return -1;
    }
    write(fd, "in", 2);
    close(fd);

    printf("Esperando tensión en el pin %s...\n", GPIO_PIN);

    // Monitorear el estado del pin GPIO
    sprintf(buf, "/sys/class/gpio/gpio%s/value", GPIO_PIN);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        perror("Error abriendo value");
        return -1;
    }

    while (1) {
        lseek(fd, 0, SEEK_SET); // Asegurarse de que estamos al principio del archivo
        nbytes = read(fd, buf, sizeof(buf));
        if (nbytes > 0) {
            buf[nbytes] = '\0';
            sscanf(buf, "%d", &value);
            if (value == 1) {
                printf("¡Tensión detectada en el pin %s!\n", GPIO_PIN);
                // Esperar hasta que la tensión desaparezca para evitar múltiples mensajes
                while (value == 1) {
                    lseek(fd, 0, SEEK_SET); // Asegurarse de que estamos al principio del archivo
                    nbytes = read(fd, buf, sizeof(buf));
                    if (nbytes > 0) {
                        buf[nbytes] = '\0';
                        sscanf(buf, "%d", &value);
                    }
                    usleep(100000); // Esperar 100ms antes de volver a comprobar
                }
                printf("Esperando tensión en el pin %s...\n", GPIO_PIN);
            }
        }
        usleep(100000); // Esperar 100ms antes de volver a comprobar
    }

    close(fd);

    // Desexportar el pin GPIO
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo /sys/class/gpio/unexport");
        return -1;
    }
    write(fd, GPIO_PIN, strlen(GPIO_PIN));
    close(fd);

    return 0;
}
