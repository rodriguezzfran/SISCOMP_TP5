# TRABAJO PRÁCTICO 5 "DEVICE DRIVER"
Este trabajo práctico sigue un poco la línea del trabajo anterior, dónde aprendimos a como crear, firmar y cargar un módulo en el kernel de nuestro sistema operativo. Ahora para este nuevo trabajo se nos encargó realizar un *__device driver__* que se encargue de sensar dos señales, adjuntamos la consigna:

### Trabajo número 5
Para superar este TP tendrán que diseñar y construir un CDD que permita sensar dos señales externas con un periodo de UN segundo. Luego una aplicación a nivel de usuario deberá leer UNA de las dos señales y graficarla en función del tiempo. La aplicación tambien debe poder indicarle al CDD cuál de las dos señales leer. Las correcciones de escalas de las mediciones, de ser necesario, se harán a nivel de usuario. Los gráficos de la señal deben indicar el tipo de señal que se
está sensando, unidades en abcisas y tiempo en ordenadas. Cuando se cambie de señal el gráfico se debe "resetear" y acomodar a la nueva medición.

Se recomienda utilizar una Raspberry Pi para desarrollar este TP.

## Marco teórico
### Driver (Controlador de Dispositivo)

**Definición:** Un driver es un programa de software que controla un dispositivo de hardware. Los drivers actúan como intermediarios entre el sistema operativo y el hardware, permitiendo que el software acceda y controle el hardware de manera eficiente y segura.

**Función:** Su función es gestionar las operaciones del dispositivo y proporcionar una interfaz para que las aplicaciones o el sistema operativo interactúen con el dispositivo.

### Bus Driver (Controlador de Bus)

**Definición:** Un bus driver es un tipo específico de driver que controla un bus de hardware. Un bus es un sistema de comunicación que transfiere datos entre componentes de una computadora.

**Ejemplos de Buses:** PCI (Peripheral Component Interconnect), USB (Universal Serial Bus), I2C (Inter-Integrated Circuit), SPI (Serial Peripheral Interface).

### Device Controller (Controlador de Dispositivo)

**Definición:** Un dispositivo controlador es un componente de hardware que maneja la operación de un periférico específico. Es responsable de enviar y recibir datos entre el dispositivo y el sistema.

**Ejemplos de Controladores:** Controlador de disco duro, controlador de pantalla, controlador de audio.

![image](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/fae0d2b0-fea5-4621-b96d-f23cb557f740)

### Character Device Driver (Controlador de Dispositivo de Caracteres)

**Definición:** Un character device driver es un tipo de controlador de dispositivo que maneja dispositivos de caracteres, los cuales permiten la transferencia de datos a y desde el dispositivo en flujos continuos de bytes (caracteres). A diferencia de los dispositivos de bloque, que manejan datos en bloques fijos, los dispositivos de caracteres se ocupan de la transmisión de datos byte por byte. Ejemplos comunes de dispositivos de caracteres incluyen terminales, impresoras y puertos seriales.

### Major y Minor Numbers

El vínculo entre APPLICATION y CDF se basa en el nombre del archivo del dispositivo. Sin embargo, el vínculo entre el CDF y DD se basa en el número del archivo de dispositivo. NO en el nombre. 
Así una app del espacio de usuario tiene cualquier nombre para el CDF, y permite que el espacio del núcleo tenga, entre el CDF/CDD, un enlace trivial basado en un índice.

**Major Number (Número Mayor):** Este número identifica al controlador de dispositivo en sí. Cada controlador tiene asignado un número mayor único, que el kernel usa para redirigir las operaciones de I/O al controlador apropiado.

**Minor Number (Número Menor):** Este número identifica un dispositivo específico que está gestionado por un controlador particular. En otras palabras, mientras que el número mayor apunta al controlador, el número menor identifica al dispositivo específico que ese controlador maneja.

Por ejemplo, si tenemos un controlador de disco con un número mayor de 8, los diferentes discos o particiones gestionados por este controlador podrían tener números menores como 0, 1, 2, etc.

Para poder ver los propios se puede ejecutar el comando `ls -l /dev` dónde podemos ver los permisos, el par major-minor y si son de tipo block o character, por ejemplo, los de una de nuestras computadoras:

![image](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/9046dbb3-70ab-4816-9724-aa2e6a8ce307)

Un hecho interesante sobre el kernel de Linux es que es una implementación de programación orientada a objetos (OO) en el lenguaje C. 

Cualquier driver de Linux consta de un constructor y un destructor. El constructor de un módulo se llama cada vez que `insmod` logra cargar el módulo en el núcleo. Por otro lado, el destructor del módulo se llama cada vez que `rmmod` logra descargar el módulo del núcleo. Estos se implementan utilizando dos funciones habituales en el driver con las macros `module_init()` y `module_exit()`, que están incluidas en el encabezado `module.h`.

Estas macros garantizan que el constructor y el destructor del módulo se registren correctamente con el kernel para su ejecución en los momentos adecuados.

## Desarrollo de un Character Device Driver

Para poder comenzar a desarrollar nuestro driver necesitamos un dispositivo que oficie de, justamente, device, así que elegimos una Raspberry pi 3 model B, la cual tiene de sistema operativo a Raspberry Pi So 64 bits (anteriormente conocido como Raspbian)
 
### Definiciones y Prototipos de Funciones

```c
#define DEVICE_NAME "CDD_GPIO_BUTTON"
#define GPIO_SIGNAL1 22
#define GPIO_SIGNAL2 27
#define BCM2837_GPIO_ADDRESS 0x3F200000

static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset);
static ssize_t device_write(struct file *file, const char *buffer, size_t len, loff_t *offset);
static int __init gpio_signal_init(void);
static void __exit gpio_signal_exit(void);
static void sample_signal(struct timer_list *timer);
static void gpio_pin_input(unsigned int pin);
```

Estas definiciones y prototipos preparan las funciones y constantes necesarias para el funcionamiento del módulo.

### Variables Globales

```c
static int major_number = 0;
static unsigned int *gpio_registers = NULL;
static int selected_signal = GPIO_SIGNAL1;
static int signal_value = 0;
static struct timer_list signal_timer;

static int gpio_pin1 = GPIO_SIGNAL1;
static int gpio_pin2 = GPIO_SIGNAL2;
```

Variables que almacenan el número mayor del dispositivo, el puntero a los registros GPIO, la señal seleccionada, el valor de la señal, el temporizador para muestreo y los pines GPIO utilizados.

### Estructura de Operaciones de Archivo

```c
static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};
```

Estructura para definir las funciones de operación para nuestro dispositivo.

### Función de Muestreo de Señal

```c
static void sample_signal(struct timer_list *timer)
{
    int new_signal_value = gpio_get_value(selected_signal);
    if (new_signal_value != signal_value) {
        signal_value = new_signal_value;
        printk(KERN_INFO "GPIO SIGNAL: Detected value %d on GPIO %d.\n", signal_value, selected_signal);
    }
    mod_timer(&signal_timer, jiffies + HZ);
}
```

Esta función se llama periódicamente para muestrear la señal del GPIO seleccionado y registrar cualquier cambio.
  - Obtiene el valor actual de la señal seleccionada usando `gpio_get_value()`.
  - Si el nuevo valor es diferente al anterior, actualiza `signal_value` y escribe un mensaje en el registro del kernel.
  - Reprograma el temporizador para que se llame de nuevo en un segundo (`HZ` representa el número de ticks por segundo).

### Función de Lectura del Dispositivo

```c
static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    unsigned int gpio_value;
    char value_str[3];
    size_t value_str_len;

    gpio_value = (*(gpio_registers + 13) & (1 << selected_signal)) != 0;
    snprintf(value_str, sizeof(value_str), "%d\n", gpio_value);
    value_str_len = strlen(value_str);

    if (*offset >= value_str_len)
        return 0;

    if (len > value_str_len - *offset)
        len = value_str_len - *offset;

    if (copy_to_user(buffer, value_str + *offset, len))
        return -EFAULT;

    *offset += len;
    
    printk(KERN_INFO "GPIO SIGNAL: Value read %u.\n", gpio_value);
    
    return len;
}
```

Lee el valor del GPIO seleccionado desde el espacio de usuario.
  - Lee el valor actual del GPIO desde los registros mapeados (`gpio_registers`).
  - Convierte el valor leído (`0` o `1`) a una cadena de caracteres (`"0\n"` o `"1\n"`).
  - Verifica si el offset actual está más allá de la longitud de la cadena.
  - Ajusta la longitud de lectura para no exceder la longitud de la cadena.
  - Copia la cadena al búfer del usuario, manejando posibles errores.
  - Actualiza el offset y escribe un mensaje en el registro del kernel.

### Función de Escritura del Dispositivo

```c
static ssize_t device_write(struct file *file, const char *buffer, size_t len, loff_t *offset)
{
    char kbuf[2];

    if (len > 1)
        len = 1;

    if (copy_from_user(kbuf, buffer, len))
        return -EFAULT;

    if (kbuf[0] == '1') {
        selected_signal = gpio_pin1;
        printk(KERN_INFO "GPIO SIGNAL: Selected GPIO %d.\n", GPIO_SIGNAL1);
    } else if (kbuf[0] == '2') {
        selected_signal = gpio_pin2;
        printk(KERN_INFO "GPIO SIGNAL: Selected GPIO %d.\n", GPIO_SIGNAL2);
    }

    return len;
}
```

permite al usuario seleccionar cuál GPIO leer escribiendo `1` o `2` en el dispositivo.
  - Limita la longitud de escritura a 1 carácter.
  - Copia el dato del espacio de usuario al búfer del kernel.
  - Si el carácter es `1`, selecciona el `GPIO_SIGNAL1`. Si es `2`, selecciona el `GPIO_SIGNAL2`.
  - Escribe un mensaje en el registro del kernel indicando el GPIO seleccionado.

### Función de Apertura del Dispositivo

```c
static int device_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "GPIO SIGNAL: Device opened.\n");
    return 0;
}
```

Esta función se llama cuando se abre el dispositivo.
  - Escribe un mensaje en el registro del kernel indicando que el dispositivo ha sido abierto.
  - Retorna 0, indicando éxito.

### Función de Liberación del Dispositivo

```c
static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "GPIO SIGNAL: Device closed.\n");
    return 0;
}
```

Esta función se llama cuando se cierra el dispositivo.
  - Escribe un mensaje en el registro del kernel indicando que el dispositivo ha sido cerrado.
  - Retorna 0, indicando éxito.

### Configuración del Pin GPIO como Entrada

```c
static void gpio_pin_input(unsigned int pin)
{
    unsigned int fsel_index = pin / 10;
    unsigned int fsel_bitpos = pin % 10;
    unsigned int *gpio_fsel = gpio_registers + fsel_index;

    printk(KERN_INFO "GPIO SIGNAL: Setting up pin %d as input.\n", pin);

    *gpio_fsel &=~ (7 << (fsel_bitpos * 3));
    *gpio_fsel |= (0 << (fsel_bitpos * 3));
    
    printk(KERN_INFO "GPIO SIGNAL: Pin %d set up as input.\n", pin);
}
```

Configura el pin GPIO especificado como entrada:
  - Calcula el índice del registro de selección de función (fsel) y la posición del bit para el pin especificado.
  - Obtiene la dirección del registro fsel correspondiente.
  - Limpia los bits correspondientes al pin en el registro fsel.
  - Establece los bits correspondientes para configurar el pin como entrada.
  - Escribe un mensaje en el registro del kernel indicando que el pin ha sido configurado como entrada.

### Función de Inicialización del Módulo

```c
static int __init gpio_signal_init(void)
{
    printk(KERN_INFO "GPIO SIGNAL: Initializing.\n");

    gpio_registers = ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);
    if (!gpio_registers) {
        printk(KERN_ALERT "GPIO SIGNAL: Failed to map GPIO memory.\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "GPIO SIGNAL: Successfully mapped GPIO memory.\n");

    if (!gpio_is_valid(GPIO_SIGNAL1) || !gpio_is_valid(GPIO_SIGNAL2)) {
        printk(KERN_ALERT "GPIO SIGNAL: Invalid GPIO pin.\n");
        iounmap(gpio_registers);
        return -ENODEV;
    }

    gpio_pin_input(gpio_pin1);
    gpio_pin_input(gpio_pin2);

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "GPIO SIGNAL: Failed to register a major number.\n");
        gpio_free(GPIO_SIGNAL1);
        gpio_free(GPIO_SIGNAL2);
        iounmap(gpio_registers);
        return major_number;
    }

    timer_setup(&signal_timer, sample_signal, 0);
    mod_timer(&signal_timer, jiffies + HZ);

    printk(KERN_INFO "GPIO SIGNAL: Registered correctly with major number %d.\n", major_number);
    return 0;
}
```

Esta función inicializa el módulo, mapea los registros GPIO, configura los pines GPIO como entradas, registra el dispositivo de carácter y configura el temporizador para el muestreo de señales.
  - Escribe un mensaje en el registro del kernel indicando que el módulo está siendo inicializado.
  - Mapea la dirección base de los registros GPIO en el espacio de direcciones del kernel.
  - Verifica si el mapeo fue exitoso, si no, escribe un mensaje de error y retorna `-ENOMEM`.
  - Verifica si los pines GPIO son válidos, si no, limpia los recursos y retorna `-ENODEV`.
  - Configura los pines GPIO como entradas usando `gpio_pin_input()`.
  - Registra el dispositivo de carácter y obtiene un número mayor, si falla limpia los recursos y retorna el código de error.
  - Configura y activa el temporizador para el muestreo de señales.
  - Escribe un mensaje en el registro del kernel indicando que el módulo se ha registrado correctamente.

### Función de Salida del Módulo

```c
static void __exit gpio_signal_exit(void)
{
    del_timer(&signal_timer);
    gpio_free(GPIO_SIGNAL1);
    gpio_free(GPIO_SIGNAL2);
    iounmap(gpio_registers);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "GPIO SIGNAL: Module unloaded.\n");
}
```

Limpia los recursos utilizados por el módulo al descargarse, incluyendo el temporizador, los pines GPIO y la memoria mapeada.
  - Elimina el temporizador.
  - Libera los pines GPIO utilizados.
  - Desmapea la memoria de los registros GPIO.
  - Desregistra el dispositivo de carácter.
  - Escribe un mensaje en el registro del kernel indicando que el módulo ha sido descargado.

El funcionamiento del script de python se ve en los videos anexados

## Ejemplos de funcionamiento

### Carga del inicial del módulo
![Captura desde 2024-06-03 16-57-39](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/3571477a-4fe0-4fbf-94e0-b1eeb8ff3fc8)

### Mensajes del anillo del kernel con cambio de pin
![Captura desde 2024-06-03 17-18-52](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/4963264c-3e47-48e4-8291-a16a3f22de11)
![Captura desde 2024-06-03 17-21-42](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/5fb6e55d-cdf3-4787-b171-198b04a43547)

### Video en funcionamiento
https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/42234d5f-dbe6-43cc-a3bd-c76ab124d57f

### Implementación Física
Para la comprobación del funcionamiento del CDD en la Raspberry Pi 3, se hizo uso de dos pines GPIO (17 y 27) configurados como entrada para poder simular el ingreso de dos señales por esos pines y mediante el CDD se hace el procesamiento y se grafica la señal recibida.
La señal, por cuestiones de simplicidad, se utilizó el pin de salida de tensión de 3V3, que mediante dos pulsadores se simulan la respuesta de activación de un sensor.
Se colocaron resistencias de 10[KΩ] en configuración Pull-Up, entonces, mientras los pulsadores permanencen sin presionarse, a la entrada de los pines hay señal 3V3 o 1 lógico, al momento de presionar el pulsador, la tensón queda en 0V o lo que es igual a 0 lógico. 

![tp5-circuito](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/103122420/9f6298e1-99eb-4d54-9394-b98b62e0ad9d)

### Circuito real
![WhatsApp Image 2024-06-09 at 21 46 08](https://github.com/rodriguezzfran/SISCOMP_TP5/assets/122646722/3e354fb4-6d6f-4390-82ba-43d8c759c7a5)


