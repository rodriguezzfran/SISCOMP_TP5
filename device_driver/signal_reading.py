import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import tkinter as tk
from tkinter import ttk

DEVICE_PATH = '/dev/CDD_GPIO_BUTTON'

# Función para leer el valor GPIO desde el dispositivo
def read_gpio():
    with open(DEVICE_PATH, 'r') as device_file:
        return int(device_file.read().strip())

# Función para seleccionar el GPIO
def select_gpio(pin):
    with open(DEVICE_PATH, 'w') as device_file:
        if pin == 1:
            device_file.write('select 1')
        elif pin == 2:
            device_file.write('select 2')

# Inicializa la selección del GPIO
pin = 1
select_gpio(pin)

# Función para cambiar el GPIO seleccionado
def change_gpio(new_pin):
    global pin, xs, ys
    if new_pin != pin:
        pin = new_pin
        select_gpio(pin)
        xs, ys = [], []  # Limpia los datos de la gráfica
        ax.set_title(f'GPIO {pin} Signal')
        print(f'Switched to GPIO {pin}')

# Función para animar la gráfica
def animate(i, xs, ys):
    signal = read_gpio()
    xs.append(time.time() - start_time)
    ys.append(signal)

    xs = xs[-100:]
    ys = ys[-100:]

    ax.clear()
    ax.plot(xs, ys)

    plt.xlabel('Time (s)')
    plt.ylabel('Signal')

# Configuración de la gráfica
fig, ax = plt.subplots()
xs, ys = [], []
start_time = time.time()

ani = animation.FuncAnimation(fig, animate, fargs=(xs, ys), interval=1000)

# Interfaz de Tkinter para seleccionar el GPIO
root = tk.Tk()
root.title("GPIO Selector")

frame = ttk.Frame(root, padding="10")
frame.grid(row=0, column=0, sticky=(tk.W, tk.E))

label = ttk.Label(frame, text="Select GPIO:")
label.grid(row=0, column=0, padx=5, pady=5)

gpio_var = tk.IntVar(value=1)
gpio1_radio = ttk.Radiobutton(frame, text="GPIO 17", variable=gpio_var, value=1, command=lambda: change_gpio(1))
gpio2_radio = ttk.Radiobutton(frame, text="GPIO 21", variable=gpio_var, value=2, command=lambda: change_gpio(2))

gpio1_radio.grid(row=0, column=1, padx=5, pady=5)
gpio2_radio.grid(row=0, column=2, padx=5, pady=5)

# Ejecuta la interfaz de Tkinter en un hilo separado
def run_tkinter():
    root.mainloop()

import threading
tk_thread = threading.Thread(target=run_tkinter)
tk_thread.start()

# Ejecuta la gráfica de Matplotlib
plt.show()
