#include <stdio.h>
#include <pigpio.h>

#define PIN 17

void setup() {
    // Inicializar pigpio
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio inicialización fallida\n");
        return;
    }

    // Configurar el pin 17 como entrada
    gpioSetMode(PIN, PI_INPUT);
    gpioSetPullUpDown(PIN, PI_PUD_DOWN); // Establecer una resistencia pull-down
}

int main(void) {
    setup();

    printf("Esperando tensión en el pin 17...\n");

    while (1) {
        // Leer el estado del pin 17
        if (gpioRead(PIN) == 1) {
            printf("¡Tensión detectada en el pin 17!\n");
            // Esperar hasta que la tensión desaparezca para evitar múltiples mensajes
            while (gpioRead(PIN) == 1) {
                time_sleep(0.1); // Esperar 100ms antes de volver a comprobar
            }
        }
        time_sleep(0.1); // Esperar 100ms antes de volver a comprobar
    }

    // Finalizar pigpio
    gpioTerminate();

    return 0;
}
