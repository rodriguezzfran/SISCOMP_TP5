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

## Desarrollo


 

