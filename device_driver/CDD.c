/**
 * gpio-button.c - A simple GPIO INPUT driver for Raspberry Pi
 * 
 * This CDD Raspberry Pi driver allows to detect a button press on a specific GPIO pin (pin 17)
 * and show a message every time the button is pressed.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define DEVICE_NAME "gpio-button"
#define GPIO_BUTTON 17

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
static int irq_number;
static int button_pressed           = 0;

// File operations structure (needed for character device)
static struct file_operations fops = {
	.open       = device_open,
	.release    = device_release,
	.read       = device_read,
};

/**
 * @brief Interrupt Service Routine for the button press.
 * 
 * This function is called when the button connected to GPIO_BUTTON is pressed.
 * 
 * @param irq Interrupt number.
 * @param data Pointer to the data (not used here).
 * @return IRQ_HANDLED if successful.
 */
static irqreturn_t button_isr(int irq, void *data)
{
    button_pressed = 1;
    printk(KERN_INFO "GPIO BUTTON: Button pressed.\n");
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
    char value_str[3];
    size_t value_str_len;

    // Write the current button state (1 or 0) to value_str
    snprintf(value_str, sizeof(value_str), "%d\n", button_pressed);
    value_str_len = strlen(value_str);
    button_pressed = 0; // Reset button pressed state after reading

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

    // Set GPIO_BUTTON as input
    if (!gpio_is_valid(GPIO_BUTTON))
    {
        printk(KERN_ALERT "GPIO BUTTON: Invalid GPIO pin.\n");
        iounmap(gpio_registers);
        return -ENODEV;
    }

    gpio_request(GPIO_BUTTON, "sysfs");
    gpio_direction_input(GPIO_BUTTON);
    gpiod_set_debounce(GPIO_BUTTON, 200);
    gpiod_export(GPIO_BUTTON, false);

    // Register interrupt handler
    irq_number = gpio_to_irq(GPIO_BUTTON);
    result = request_irq(irq_number, (irq_handler_t) button_isr, IRQF_TRIGGER_RISING, DEVICE_NAME, NULL);
    if (result)
    {
        printk(KERN_ALERT "GPIO BUTTON: Failed to request IRQ.\n");
        gpio_free(GPIO_BUTTON);
        iounmap(gpio_registers);
        return result;
    }

    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "GPIO BUTTON: Failed to register a major number.\n");
        free_irq(irq_number, NULL);
        gpio_free(GPIO_BUTTON);
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
    free_irq(irq_number, NULL);
    gpiod_unexport(GPIO_BUTTON);
    gpio_free(GPIO_BUTTON);
    iounmap(gpio_registers);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "GPIO BUTTON: Module unloaded.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tu Nombre");
MODULE_DESCRIPTION("A simple GPIO INPUT driver for Raspberry Pi to detect button press");
MODULE_VERSION("1.0");

module_init(gpio_button_init);
module_exit(gpio_button_exit);
