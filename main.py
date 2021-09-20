import os
import tkinter as tk
import tkinter.ttk as ttk
import pygubu
import serial.tools.list_ports

PROJECT_PATH = os.path.abspath(os.path.dirname(__file__))
PROJECT_UI = os.path.join(PROJECT_PATH, "LCR_Software.ui")

class LcrSoftwareApp:
    def __init__(self, master=None):
        self.builder = builder = pygubu.Builder()
        builder.add_resource_path(PROJECT_PATH)
        builder.add_from_file(PROJECT_UI)
        self.mainwindow = builder.get_object('mainwindow', master)

        self.primaryentry = None
        self.primarylabel = None
        self.secondaryentry = None
        self.secondarylabel = None
        self.stop = None
        self.start = None
        self.measurementbox = None
        self.measurementlabel = None
        self.frequencybox = None
        self.secondarybox = None
        self.primarybox = None
        self.frequencylabel = None
        self.comlabel = None
        self.combox = None
        builder.import_variables(self, ['primaryentry', 'primarylabel', 'secondaryentry', 'secondarylabel', 'stop', 'start', 'measurementbox', 'measurementlabel', 'frequencybox', 'secondarybox', 'primarybox', 'frequencylabel', 'comlabel', 'combox'])

        builder.connect_callbacks(self)

    def callback(self, event=None):
        pass

    def stop(self):
        pass

    def start(self):
        pass

    def run(self):
        self.mainwindow.mainloop()

    def serialports(self):
        return serial.tools.list_ports.comports()


if __name__ == '__main__':
    root = tk.Tk()
    app = LcrSoftwareApp(root)
    app.run()

