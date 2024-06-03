#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/timer.h>

#define DEVICE_NAME "CDD_GPIO_BUTTON"
#define BUFFER_SIZE 256

static dev_t first;       
static struct cdev c_dev; 
static struct class *cl;  

static char message[BUFFER_SIZE] = {0};
static short message_size;
static int selected_gpio = 17; // Pin GPIO seleccionado por defecto
static int gpio_pin1 = 17;     
static int gpio_pin2 = 21;     

static struct timer_list timer;
static int gpio_value;
static bool timer_started = false;

// Función del temporizador para leer el GPIO cada segundo
static void timer_callback(struct timer_list *t) {
    gpio_value = gpio_get_value(selected_gpio);
    mod_timer(&timer, jiffies + HZ);
}

// Función de apertura del dispositivo
static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Dispositivo abierto.\n");
    if (!timer_started) {
        timer_started = true;
        timer_setup(&timer, timer_callback, 0);
        mod_timer(&timer, jiffies + HZ);
    }
    return 0;
}

// Función de liberación del dispositivo
static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Dispositivo cerrado.\n");
    return 0;
}

// Función de lectura del dispositivo
static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    char gpio_value_str[2];
    memset(message, 0, sizeof(message));  
    sprintf(gpio_value_str, "%d", gpio_value);
    strcat(message, gpio_value_str);
    strcat(message, "\n");

    message_size = strlen(message);

    if (*offset >= message_size)
        return 0;

    if (length > message_size - *offset)
        length = message_size - *offset;

    if (copy_to_user(buffer, message + *offset, length) != 0)
        return -EFAULT;

    *offset += length;
    return length;
}

// Función de escritura en el dispositivo
static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    char command[BUFFER_SIZE];
    if (copy_from_user(command, buffer, length) != 0)
        return -EFAULT;

    if (length > 0) {
        if (strncmp(command, "select 1", 8) == 0) {
            selected_gpio = gpio_pin1;
            printk(KERN_INFO "GPIO 17 seleccionado.\n");
        } else if (strncmp(command, "select 2", 8) == 0) {
            selected_gpio = gpio_pin2;
            printk(KERN_INFO "GPIO 21 seleccionado.\n");
        } else {
            printk(KERN_INFO "Comando no válido.\n");
        }
    }
    return length;
}

// Estructura de operaciones del dispositivo
static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write
};

// Función de inicialización del módulo
static int __init gpio_device_init(void) {
    int ret;
    struct device *dev_ret;

    printk(KERN_INFO "Inicializando módulo de dispositivo GPIO.\n");

    if ((ret = alloc_chrdev_region(&first, 0, 1, DEVICE_NAME)) < 0) {
        printk(KERN_ALERT "Error al registrar el número principal de dispositivo.\n");
        return ret;
    }

    if (IS_ERR(cl = class_create(THIS_MODULE, "chardrv"))) {
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cl);
    }

    if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, DEVICE_NAME))) {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    if (gpio_request(gpio_pin1, "GPIO_PIN1") < 0 || gpio_request(gpio_pin2, "GPIO_PIN2") < 0) {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -ENODEV;
    }

    gpio_direction_input(gpio_pin1);
    gpio_direction_input(gpio_pin2);

    cdev_init(&c_dev, &fops);
    if ((ret = cdev_add(&c_dev, first, 1)) < 0) {
        gpio_free(gpio_pin1);
        gpio_free(gpio_pin2);
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return ret;
    }

    return 0;
}

// Función de limpieza del módulo
static void __exit gpio_device_exit(void) {
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    gpio_free(gpio_pin1);
    gpio_free(gpio_pin2);
    del_timer(&timer);
    printk(KERN_INFO "Módulo de dispositivo GPIO desregistrado.\n");
}

module_init(gpio_device_init);
module_exit(gpio_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Internautas");
MODULE_DESCRIPTION("Character device driver para control de GPIO en Raspberry Pi 3 model B");
