#include <wiringPi.h>
#include <stdio.h>

#define PIN 17

void setup() {
    // Inicializar wiringPi y configurar el pin 17 como entrada
    wiringPiSetupGpio(); // Usa la numeración BCM
    pinMode(PIN, INPUT);
    pullUpDnControl(PIN, PUD_DOWN); // Establecer una resistencia pull-down
}

int main(void) {
    setup();

    printf("Esperando tensión en el pin 17...\n");

    while (1) {
        // Leer el estado del pin 17
        if (digitalRead(PIN) == HIGH) {
            printf("¡Tensión detectada en el pin 17!\n");
            // Esperar hasta que la tensión desaparezca para evitar múltiples mensajes
            while (digitalRead(PIN) == HIGH) {
                delay(100); // Esperar 100ms antes de volver a comprobar
            }
        }
        delay(100); // Esperar 100ms antes de volver a comprobar
    }

    return 0;
}
