import tkinter as tk
from tkinter import ttk
import time
import threading
import os
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np

# Global variables
running = True
selected_signal = 1
signal_data = []

# Function to read signal from the CDD
def read_signal():
    global running, signal_data
    while running:
        with open('/dev/CDD_GPIO_SIGNAL', 'r') as f:
            value = int(f.read().strip())
            signal_data.append(value)
        time.sleep(1)

# Function to update the selected signal in the CDD
def set_signal(signal):
    global selected_signal
    selected_signal = signal
    with open('/dev/CDD_GPIO_SIGNAL', 'w') as f:
        f.write(str(signal))

# Function to update the plot
def update_plot():
    global signal_data
    if len(signal_data) > 100:
        signal_data = signal_data[-100:]

    x = np.arange(len(signal_data))
    y = np.array(signal_data)

    ax.clear()
    ax.plot(x, y, label=f'Signal {selected_signal}')
    ax.set_title(f'Signal {selected_signal} over Time')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Signal Value')
    ax.legend()
    canvas.draw()

    if running:
        root.after(1000, update_plot)

# Function to handle signal selection from the GUI
def on_signal_select(event):
    signal = signal_selector.get()
    set_signal(int(signal.split()[1]))

# Function to handle closing the application
def on_closing():
    global running
    running = False
    root.quit()

# Initialize the main window
root = tk.Tk()
root.title("GPIO Signal Monitor")

# Create a dropdown to select the signal
signal_selector = ttk.Combobox(root, values=["Signal 1", "Signal 2"])
signal_selector.set("Signal 1")
signal_selector.bind("<<ComboboxSelected>>", on_signal_select)
signal_selector.pack(pady=10)

# Create the matplotlib figure and axis
fig, ax = plt.subplots()
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(fill=tk.BOTH, expand=1)

# Start the thread to read the signal
threading.Thread(target=read_signal, daemon=True).start()

# Start the periodic plot update
root.after(1000, update_plot)

# Handle window closing
root.protocol("WM_DELETE_WINDOW", on_closing)

# Start the Tkinter main loop
root.mainloop()