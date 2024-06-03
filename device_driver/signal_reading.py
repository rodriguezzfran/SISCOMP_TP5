import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time
import os

class GPIOApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Real-Time GPIO Signal Plotter")
        self.signal_pin = 1
        self.running = False

        self.create_widgets()
        self.setup_plot()

    def create_widgets(self):
        frame = ttk.Frame(self.root)
        frame.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)

        self.label = ttk.Label(frame, text="Select Signal:")
        self.label.pack(side=tk.LEFT)

        self.signal_var = tk.StringVar(value="1")
        self.signal_selector = ttk.Combobox(frame, textvariable=self.signal_var, values=["1", "2"])
        self.signal_selector.pack(side=tk.LEFT, padx=5)
        self.signal_selector.bind("<<ComboboxSelected>>", self.change_signal)

        self.start_button = ttk.Button(frame, text="Start", command=self.start_plotting)
        self.start_button.pack(side=tk.LEFT, padx=5)

        self.stop_button = ttk.Button(frame, text="Stop", command=self.stop_plotting, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=5)

    def setup_plot(self):
        self.figure, self.ax = plt.subplots()
        self.canvas = FigureCanvasTkAgg(self.figure, master=self.root)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self.ax.set_title("Real-Time GPIO Signal")
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Signal Value")
        self.line, = self.ax.plot([], [])

    def change_signal(self, event=None):
        self.signal_pin = int(self.signal_var.get())
        self.ax.set_title(f"Real-Time GPIO Signal (Pin {self.signal_pin})")
        self.reset_plot()

    def start_plotting(self):
        self.running = True
        self.start_button.config(state=tk.DISABLED)
        self.stop_button.config(state=tk.NORMAL)
        self.reset_plot()
        self.plot_thread = threading.Thread(target=self.plot_signal)
        self.plot_thread.start()

    def stop_plotting(self):
        self.running = False
        self.start_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)

    def reset_plot(self):
        self.ax.clear()
        self.ax.set_title(f"Real-Time GPIO Signal (Pin {self.signal_pin})")
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Signal Value")
        self.line, = self.ax.plot([], [])
        self.canvas.draw()

    def plot_signal(self):
        data = []
        start_time = time.time()
        while self.running:
            signal_value = self.read_signal(self.signal_pin)
            current_time = time.time() - start_time
            data.append((current_time, signal_value))
            times, values = zip(*data)
            self.line.set_xdata(times)
            self.line.set_ydata(values)
            self.ax.relim()
            self.ax.autoscale_view()
            self.canvas.draw()
            time.sleep(1)

    def read_signal(self, pin):
        try:
            with open(f"/dev/CDD_GPIO_BUTTON", "w") as f:
                f.write(str(pin))
            with open(f"/dev/CDD_GPIO_BUTTON", "r") as f:
                signal_value = int(f.read().strip())
        except Exception as e:
            print(f"Error reading signal: {e}")
            signal_value = 0
        return signal_value

if __name__ == "__main__":
    root = tk.Tk()
    app = GPIOApp(root)
    root.mainloop()
