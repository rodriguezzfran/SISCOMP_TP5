#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define GPIO_PIN "17"  // Pin GPIO que vamos a usar

void export_gpio(const char *pin) {
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo /sys/class/gpio/export");
        exit(EXIT_FAILURE);
    }
    write(fd, pin, strlen(pin));
    close(fd);
}

void set_gpio_direction(const char *pin, const char *direction) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo direction");
        exit(EXIT_FAILURE);
    }
    write(fd, direction, strlen(direction));
    close(fd);
}

int read_gpio_value(const char *pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Error abriendo value");
        exit(EXIT_FAILURE);
    }
    char value_str[3];
    if (read(fd, value_str, sizeof(value_str)) > 0) {
        close(fd);
        return atoi(value_str);
    }
    close(fd);
    return -1;
}

void unexport_gpio(const char *pin) {
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        perror("Error abriendo /sys/class/gpio/unexport");
        exit(EXIT_FAILURE);
    }
    write(fd, pin, strlen(pin));
    close(fd);
}

int main(void) {
    export_gpio(GPIO_PIN);
    set_gpio_direction(GPIO_PIN, "in");

    printf("Esperando tensión en el pin %s...\n", GPIO_PIN);

    while (1) {
        int value = read_gpio_value(GPIO_PIN);
        if (value == 1) {
            printf("¡Tensión detectada en el pin %s!\n", GPIO_PIN);
            while (value == 1) {
                usleep(100000); // Esperar 100ms antes de volver a comprobar
                value = read_gpio_value(GPIO_PIN);
            }
            printf("Esperando tensión en el pin %s...\n", GPIO_PIN);
        }
        usleep(100000); // Esperar 100ms antes de volver a comprobar
    }

    unexport_gpio(GPIO_PIN);
    return 0;
}

