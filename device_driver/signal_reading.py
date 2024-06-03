import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time

# Leer desde el archivo del dispositivo en /dev/signal-detect
def read_driver_file():
    with open('/dev/gpio-button', 'r') as file:
        data = file.read().strip()
    # Suponemos que el archivo contiene dos valores separados por espacio
    signal1, signal2 = map(int, data.split())
    return signal1, signal2

class SignalApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Signal Viewer")
        self.geometry("800x600")
        
        self.signal_option = tk.StringVar(value="Signal 1")
        self.running = True
        
        self.create_widgets()
        self.start_plotting_thread()
    
    def create_widgets(self):
        # Frame para los botones de selección
        control_frame = ttk.Frame(self)
        control_frame.pack(side=tk.TOP, fill=tk.X)

        ttk.Label(control_frame, text="Select Signal:").pack(side=tk.LEFT, padx=10, pady=10)
        
        self.signal1_button = ttk.Radiobutton(control_frame, text="Signal 1", variable=self.signal_option, value="Signal 1", command=self.reset_plot)
        self.signal1_button.pack(side=tk.LEFT, padx=10, pady=10)

        self.signal2_button = ttk.Radiobutton(control_frame, text="Signal 2", variable=self.signal_option, value="Signal 2", command=self.reset_plot)
        self.signal2_button.pack(side=tk.LEFT, padx=10, pady=10)

        # Frame para la gráfica
        self.fig, self.ax = plt.subplots()
        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.ax.set_xlabel('Time (s)')
        self.ax.set_ylabel('Signal Value')

    def start_plotting_thread(self):
        self.plot_thread = threading.Thread(target=self.update_plot, daemon=True)
        self.plot_thread.start()

    def update_plot(self):
        x_data = []
        y_data = []

        start_time = time.time()

        while self.running:
            current_time = time.time() - start_time
            signal1, signal2 = read_driver_file()

            if self.signal_option.get() == "Signal 1":
                y_value = signal1
            else:
                y_value = signal2
            
            x_data.append(current_time)
            y_data.append(y_value)

            self.ax.clear()
            self.ax.plot(x_data, y_data, label=self.signal_option.get())
            self.ax.legend()
            self.ax.set_xlabel('Time (s)')
            self.ax.set_ylabel('Signal Value')
            self.canvas.draw()

            time.sleep(0.1)

            # Limitar la longitud de los datos para no sobrecargar la gráfica
            if len(x_data) > 100:
                x_data.pop(0)
                y_data.pop(0)

    def reset_plot(self):
        self.ax.clear()
        self.canvas.draw()

    def on_closing(self):
        self.running = False
        self.plot_thread.join()
        self.destroy()

if __name__ == "__main__":
    app = SignalApp()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()
