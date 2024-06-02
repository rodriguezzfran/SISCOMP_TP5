#include <linux/module.h>        // Para módulos de kernel
#include <linux/kernel.h>        // Para printk() y otros
#include <linux/init.h>          // Para las macros __init y __exit
#include <linux/gpio.h>          // Para GPIO functions
#include <linux/interrupt.h>     // Para manejo de interrupciones
#include <linux/kthread.h>       // Para manejo de hilos del kernel
#include <linux/delay.h>         // Para funciones de delay

#define GPIO_BUTTON 17  // Definir el GPIO del botón

// Guardar el número de interrupción asociado al GPIO
static int interrupt_number;

// Guardar el hilo del kernel
static struct task_struct *task_thread;

// Función que se ejecuta cuando se presiona el botón
static irqreturn_t button_isr(int irq, void *data)
{
    int button_state = gpio_get_value(GPIO_BUTTON);
    printk(KERN_INFO "Button ISR state: %d\n", button_state);
    return IRQ_HANDLED;
}

// Función que se ejecuta en el hilo del kernel
static int signal_thread_task(void *arg)
{
    while (!kthread_should_stop())
    {
        int button_state = gpio_get_value(GPIO_BUTTON);
        printk(KERN_INFO "Button thread state: %d\n", button_state);
        msleep(200); // Espera 200 ms para volver a leer el estado del botón para evitar rebotes
    }
    return 0;
}

// Inicialización del módulo
static int __init cdd_init(void)
{
    int result;

    if (!gpio_is_valid(GPIO_BUTTON))
    {
        printk(KERN_INFO "GPIO %d no es válido\n", GPIO_BUTTON);
        return -ENODEV;
    }

    // Configura el GPIO como entrada
    if (gpio_request(GPIO_BUTTON, "sysfs") < 0)
    {
        printk(KERN_INFO "Error al solicitar GPIO %d\n", GPIO_BUTTON);
        return -ENODEV;
    }
    gpio_direction_input(GPIO_BUTTON);
    gpio_set_debounce(GPIO_BUTTON, 200);
    gpio_export(GPIO_BUTTON, false);

    // Registra la interrupción
    interrupt_number = gpio_to_irq(GPIO_BUTTON);
    result = request_irq(interrupt_number, button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button_gpio_handler", NULL);
    if (result)
    {
        printk(KERN_INFO "Error al solicitar interrupción %d\n", interrupt_number);
        gpio_unexport(GPIO_BUTTON);
        gpio_free(GPIO_BUTTON);
        return result;
    }

    // Crea el hilo del kernel
    task_thread = kthread_run(signal_thread_task, NULL, "signal_thread_task");
    if (IS_ERR(task_thread))
    {
        printk(KERN_INFO "Error al crear el hilo del kernel\n");
        free_irq(interrupt_number, NULL);
        gpio_unexport(GPIO_BUTTON);
        gpio_free(GPIO_BUTTON);
        return PTR_ERR(task_thread);
    }

    printk(KERN_INFO "Módulo CDD cargado\n");
    return 0;
}

// Función de descarga del módulo
static void __exit cdd_exit(void)
{
    kthread_stop(task_thread);
    free_irq(interrupt_number, NULL);
    gpio_unexport(GPIO_BUTTON);
    gpio_free(GPIO_BUTTON);
    printk(KERN_INFO "Módulo CDD descargado\n");
}

module_init(cdd_init);
module_exit(cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Franco Rodriguez, Mauricio Valdez, Bruno Guglielmotti");
MODULE_DESCRIPTION("Módulo de kernel para manejo de botón con interrupciones y hilo del kernel");
MODULE_VERSION("1.0");