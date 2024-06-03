/**
 * gpio-button.c - A simple GPIO INPUT driver for Raspberry Pi
 * 
 * This CDD Raspberry Pi driver allows to detect a button press on specific GPIO pins (pin 17 and pin 27)
 * and show a message every time a button is pressed.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define DEVICE_NAME "CDD_GPIO_BUTTON"
#define GPIO_BUTTON1 17
#define GPIO_BUTTON2 27

// Define GPIO base addresses
#define BCM2837_GPIO_ADDRESS 0x3F200000

// Function prototypes
static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset);
static irqreturn_t button_isr(int irq, void *data);
static int __init gpio_button_init(void);
static void __exit gpio_button_exit(void);

// Global variables
static int major_number             = 0;
static unsigned int *gpio_registers = NULL;
static int irq_number1, irq_number2;
static int button_pressed1          = 0;
static int button_pressed2          = 0;

// File operations structure (needed for character device)
static struct file_operations fops = {
    .open       = device_open,
    .release    = device_release,
    .read       = device_read,
};

/**
 * @brief Interrupt Service Routine for the button press.
 * 
 * This function is called when the button connected to GPIO_BUTTON1 or GPIO_BUTTON2 is pressed.
 * 
 * @param irq Interrupt number.
 * @param data Pointer to the data (not used here).
 * @return IRQ_HANDLED if successful.
 */
static irqreturn_t button_isr(int irq, void *data)
{
    if (irq == irq_number1) {
        button_pressed1 = 1;
        printk(KERN_INFO "GPIO BUTTON: Button 1 pressed.\n");
    } else if (irq == irq_number2) {
        button_pressed2 = 1;
        printk(KERN_INFO "GPIO BUTTON: Button 2 pressed.\n");
    }
    return IRQ_HANDLED;
}

/**
 * @brief Device read function.
 * 
 * This function is called when the device file is read to get the button press status.
 * 
 * @param file Pointer to the file structure.
 * @param buffer Pointer to the buffer where the data will be written.
 * @param len Length of the buffer.
 * @param offset Pointer to the offset in the file.
 * @return Number of bytes read.
 */
static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    char value_str[10];
    size_t value_str_len;

    // Write the current button states (1 or 0) to value_str
    snprintf(value_str, sizeof(value_str), "%d %d\n", button_pressed1, button_pressed2);
    value_str_len = strlen(value_str);
    button_pressed1 = 0; // Reset button pressed state after reading
    button_pressed2 = 0; // Reset button pressed state after reading

    // If offset is greater than or equal to the length of the string, return 0
    if (*offset >= value_str_len)
        return 0;

    // If len is greater than the remaining data, adjust len
    if (len > value_str_len - *offset)
        len = value_str_len - *offset;

    // Copy data to user space
    if (copy_to_user(buffer, value_str + *offset, len))
        return -EFAULT;

    *offset += len;
    
    return len;
}

/**
 * @brief Device open function.
 *
 * This function is called when the device file is opened.
 *
 * @param inode Pointer to the inode structure.
 * @param file Pointer to the file structure.
 * @return 0 on success.
 */
static int device_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "GPIO BUTTON: Device opened.\n");
    return 0;
}

/**
 * @brief Device release function.
 *
 * This function is called when the device file is closed.
 *
 * @param inode Pointer to the inode structure.
 * @param file Pointer to the file structure.
 * @return 0 on success.
 */
static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "GPIO BUTTON: Device closed.\n");
    return 0;
}

/**
 * @brief Module initialization function.
 *
 * This function is called when the module is loaded. It initializes the
 * GPIO registers and registers the character device.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int __init gpio_button_init(void)
{
    int result;

    printk(KERN_INFO "GPIO BUTTON: Initializing.\n");

    // Map GPIO memory
    gpio_registers = (unsigned int *)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);
    if (!gpio_registers)
    {
        printk(KERN_ALERT "GPIO BUTTON: Failed to map GPIO memory.\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "GPIO BUTTON: Successfully mapped GPIO memory.\n");

    // Set GPIO_BUTTON1 and GPIO_BUTTON2 as input
    if (!gpio_is_valid(GPIO_BUTTON1) || !gpio_is_valid(GPIO_BUTTON2))
    {
        printk(KERN_ALERT "GPIO BUTTON: Invalid GPIO pin.\n");
        iounmap(gpio_registers);
        return -ENODEV;
    }

    gpio_request(GPIO_BUTTON1, "sysfs");
    gpio_direction_input(GPIO_BUTTON1);
    gpiod_set_debounce(GPIO_BUTTON1, 200);
    gpiod_export(GPIO_BUTTON1, false);

    gpio_request(GPIO_BUTTON2, "sysfs");
    gpio_direction_input(GPIO_BUTTON2);
    gpiod_set_debounce(GPIO_BUTTON2, 200);
    gpiod_export(GPIO_BUTTON2, false);

    // Register interrupt handlers
    irq_number1 = gpio_to_irq(GPIO_BUTTON1);
    irq_number2 = gpio_to_irq(GPIO_BUTTON2);

    result = request_irq(irq_number1, (irq_handler_t) button_isr, IRQF_TRIGGER_RISING, DEVICE_NAME, NULL);
    if (result)
    {
        printk(KERN_ALERT "GPIO BUTTON: Failed to request IRQ for GPIO_BUTTON1.\n");
        gpio_free(GPIO_BUTTON1);
        gpio_free(GPIO_BUTTON2);
        iounmap(gpio_registers);
        return result;
    }

    result = request_irq(irq_number2, (irq_handler_t) button_isr, IRQF_TRIGGER_RISING, DEVICE_NAME, NULL);
    if (result)
    {
        printk(KERN_ALERT "GPIO BUTTON: Failed to request IRQ for GPIO_BUTTON2.\n");
        free_irq(irq_number1, NULL);
        gpio_free(GPIO_BUTTON1);
        gpio_free(GPIO_BUTTON2);
        iounmap(gpio_registers);
        return result;
    }

    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "GPIO BUTTON: Failed to register a major number.\n");
        free_irq(irq_number1, NULL);
        free_irq(irq_number2, NULL);
        gpio_free(GPIO_BUTTON1);
        gpio_free(GPIO_BUTTON2);
        iounmap(gpio_registers);
        return major_number;
    }

    printk(KERN_INFO "GPIO BUTTON: Registered correctly with major number %d.\n", major_number);
    return 0;
}

/**
 * @brief Module exit function.
 *
 * This function is called when the module is unloaded. It unregisters the
 * character device and unmaps the GPIO memory.
 */
static void __exit gpio_button_exit(void)
{
    free_irq(irq_number1, NULL);
    free_irq(irq_number2, NULL);
    gpiod_unexport(GPIO_BUTTON1);
    gpiod_unexport(GPIO_BUTTON2);
    gpio_free(GPIO_BUTTON1);
    gpio_free(GPIO_BUTTON2);
    iounmap(gpio_registers);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "GPIO BUTTON: Module unloaded.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Franco Rodriguez, Mauricio Valdez, Bruno Guglielmotti");
MODULE_DESCRIPTION("A simple GPIO INPUT driver for Raspberry Pi to detect button presses on multiple pins");
MODULE_VERSION("1.0");

module_init(gpio_button_init);
module_exit(gpio_button_exit);
