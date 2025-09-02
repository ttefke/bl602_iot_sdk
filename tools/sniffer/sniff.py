import errno
import os
import serial
import stat
import struct
import time
import threading
import tkinter as tk
from tkinter.filedialog import asksaveasfile
from tkinter.scrolledtext import ScrolledText
from tkinter import ttk

class Application(tk.Tk):
    def __init__(self):
        # Init window
        super().__init__()

        # Apply new title
        self.title("BL602 Monitor")

        # Set height, width and position
        screenWidth = self.winfo_screenwidth()
        screenHeight = self.winfo_screenheight()
        rootWidth = int(screenHeight * 19 / 9 / 2)
        rootHeight = int(screenHeight / 2)
        centerX = int(screenWidth/2 - rootWidth/2)
        centerY = int(screenHeight/2 - rootHeight/2)
        self.geometry(f"{rootWidth}x{rootHeight}+{centerX}+{centerY}")

        # Create menu bar
        menubar = tk.Menu(self)
        self.config(menu=menubar)
        fileMenu = tk.Menu(menubar, tearoff=False)
        fileMenu.add_command(label="Save", command=self.saveToFile)
        fileMenu.add_command(label="Exit", command=self.exit)
        menubar.add_cascade(label="File", menu=fileMenu, underline=0)

        # Override x button
        self.protocol("WM_DELETE_WINDOW", self.exit)

        # Create serial text output view
        self.text = ScrolledText(self, width=80, height=15)
        self.text.configure(state="disabled")
        self.text.pack(padx = 10, pady = (10, 5), fill=tk.BOTH, expand=True, anchor=tk.CENTER)

        # Create serial text input
        self.serialInputEntry = ttk.Entry(self)
        self.serialInputEntry.pack(padx = (10, 5), pady=(5, 10), side=tk.LEFT, fill=tk.BOTH, expand=True, anchor=tk.S)
        self.serialInputEntry.bind("<Return>", self.sendSerialBind) # Send text when pressing return

        # Create send serial text button
        self.sendSerialInput = ttk.Button(self, text="Send", command=self.sendSerial)
        self.sendSerialInput.pack(padx = (5, 10), pady = (5, 10), fill=tk.X, anchor=tk.SE)

        # Create PCAP output object
        self.pcapOutput = BinaryToPCAP()

        # Start reading serial console
        self.serialConsoleReading = 1
        thread = threading.Thread(target=self.readSerial, name="Serial monitor thread")
        thread.start()

        # Buffer to write data
        self.serialOutputBuffer = ""

    def saveToFile(self):
        # Open save file dialog
        fileName = asksaveasfile(mode="w", defaultextension=".txt", filetypes=[("Text documents", "*.txt")])

        # Dialog dismissed -> return
        if fileName is None:
            return
        
        # Store data
        text = str(self.text.get(1.0, tk.END))
        fileName.write(text)
        fileName.close()

    def readSerial(self):
        # Default configuration
        conf = {
            "device": "/dev/ttyUSB0",
            "baudrate": 2000000
        }

        try:
            with serial.Serial(conf["device"], conf["baudrate"],
                bytesize=serial.EIGHTBITS, parity = serial.PARITY_NONE,
                stopbits = serial.STOPBITS_ONE, xonxoff = False, rtscts= False, timeout=1) as ser:

                ser.reset_input_buffer()
                ser.reset_output_buffer()

                while self.serialConsoleReading:
                    # Write output data from self.serialOutputBuffer if present
                    if len(self.serialOutputBuffer) > 0:
                        # Encode into ascii
                        serialString = self.serialOutputBuffer.encode('utf-8')

                        # Clear buffer
                        self.serialOutputBuffer = ""

                        # Send text
                        test = ser.write(serialString)
                    
                    # Read line
                    line = ser.readline()
                    line = line.strip()
                    lineLen = len(line)

                    # Write line to output if text is present
                    if lineLen > 1:
                        try:
                            line = line.decode("utf-8")

                            # Check if line is packet
                            if line.startswith("#pkt#"):
                                self.pcapOutput.handleReceivedFrame(bytes(line[5:], encoding="utf-8"))
                            else:
                                # Output line
                                line = line + "\n"
                                self.text.configure(state="normal")
                                self.text.insert("end", line)
                                self.text.configure(state="disabled")
                                self.text.see("end")
                        except UnicodeDecodeError as e:
                            print(f"Could not decode line: {line}")
                            print(f"Exception: {e}")
        except KeyboardInterrupt as e:
            print("Bye")
        except Exception as e:
            print("Could not open port: ", e)


    # Binding to send text if enter is pressed
    def sendSerialBind(self, textEntry):
        self.sendSerial()

    # Binding to send text if button is pressed
    def sendSerial(self):
        # Get current content and sent store in buffer which is evaluated in read function
        self.serialOutputBuffer = self.serialInputEntry.get() + "\r\n"

        # Clear input entry
        self.serialInputEntry.delete(0, tk.END)

    # Exit function
    def exit(self):
        self.serialConsoleReading = 0 # Stop reading serial console
        self.destroy() # Destroy GUI

# motivated from https://github.com/g-oikonomou/sensniff/blob/master/sensniff.py
class BinaryToPCAP():
    def __init__(self):
        # Generate header https://wiki.wireshark.org/Development/LibpcapFileFormat

        # Constants
        self.DLT_IEEE802_11 = 105
        self.PCAP_GLOBAL_HEADER_FORMAT = '<LHHlLLL'
        self.PCAP_FRAME_HEADER_FORMAT = '<LLLL'

        PCAP_MAGIC_NUMBER = 0xA1B2C3D4
        PCAP_VERSION_MAJOR = 2
        PCAP_VERSION_MINOR = 4
        PCAP_TIMEZONE = 0
        PCAP_SIGFIGS = 0
        PCAP_SNAPLEN = 65535
        PCAP_NETWORK = self.DLT_IEEE802_11

        self.outputFifoPath = "/tmp/sniff"

        # Create global header
        self.PCAP_GLOBAL_HEADER = bytearray(struct.pack(self.PCAP_GLOBAL_HEADER_FORMAT,
            PCAP_MAGIC_NUMBER,
            PCAP_VERSION_MAJOR,
            PCAP_VERSION_MINOR,
            PCAP_TIMEZONE,
            PCAP_SIGFIGS,
            PCAP_SNAPLEN,
            PCAP_NETWORK))

        self.createdFifo = False
        self.outputFifo = None
        self.requiresPCAPHeader = True
        self.createFifo()

    def createFifo(self):
        # Create fifo queue
        try:
            if not self.createdFifo:
                self.createdFifo = True
                os.mkfifo(self.outputFifoPath)
        except OSError as e:
            if e.errno == errno.EEXIST:
                if stat.S_ISFIFO(os.stat(self.outputFifoPath).st_mode) is False:
                    print(f"File {self.outputFifoPath} exists and is not a FIFO queue")
                else:
                    print(f"Queue {self.outputFifoPath} already exists - reusing it")
            else:
                print(f"Can not create fifo queue at {self.outputFifoPath}: {e}")
                os.exit(1)

    def openFifoQueue(self):
        # Open fifo queue
        if self.outputFifo is None:
            try:
                fileDescriptor = os.open(self.outputFifoPath, os.O_NONBLOCK | os.O_WRONLY)
                self.outputFifo = os.fdopen(fileDescriptor, "wb")
            except OSError as e:
                print("Could not open fifo queue, is Wireshark reading?")
                self.outputFifo = None
                self.requiresPCAPHeader = True

    def handleReceivedFrame(self, raw):
        # Create timestamp
        timestamp = time.time()
        seconds = int(timestamp)
        microseconds = int((timestamp - seconds) * 1000000)

        # Measure length of received frame
        length = len(raw)

        # Generate PCAP header
        header = struct.pack(self.PCAP_FRAME_HEADER_FORMAT, seconds, microseconds, length, length)

        # Generate PCAP
        pcap = bytearray(header) + raw
        
        # Pipe PCAP
        if self.createdFifo is False:
            self.createFifo()

        if self.outputFifo is None:
            self.openFifoQueue()
        
        if self.outputFifo is not None:
            try:
                if self.requiresPCAPHeader is True:
                    self.requiresPCAPHeader = False
                    self.outputFifo.write(self.PCAP_GLOBAL_HEADER)

                self.outputFifo.write(pcap)
                self.outputFifo.flush()
            except IOError as e:
                if e.errno == errno.EPIPE:
                    print("Remote end stopped reading")
                    self.outputFifo = None
                    self.requiresPCAPHeader = True


if __name__ == "__main__":
    app = Application()
    app.mainloop()