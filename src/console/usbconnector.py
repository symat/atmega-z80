from serial.tools.list_ports import comports
from serial import Serial
import sys

USB_VID = int("0x1209", base=16)
USB_PID = int("0x80A0", base=16)
USB_DESC = "ATMega-Z80 hobby computer" # maximum 30 chars

class ATMegaZ80UsbConnector:
    def __init__(self, port=None, baud_rate=9600):
        if port is None:
            self.port = PortDetector().port
        else:
            self.port = port
        
        # the data bit, parity and stop bit settings are defined in 
        # he MCP2221A data sheet, the chip only allows these values
        self.data_bits = 8
        self.parity = False
        self.stop_bit = 1


        with Serial(self.port, 19200, timeout=1) as ser:
            ser.write(b'GET_LINE_CODING\n') 

            #x = ser.read()          # read one byte
            #s = ser.read(10)        # read up to ten bytes (timeout)
            line = ser.readline()   # read a '\n' terminated line
            print("response: " + line)


class PortDetector:
    def __init__(self):
        all_ports = comports()
        if not self.auto_detect(all_ports):
            self.select_port_or_exit(all_ports)

    def select_port_or_exit(self, all_ports):
        for idx, port in enumerate(all_ports):
            print(f"[{idx}] :   {port.device}")
            print(f"          desc: {port.description}")
            print(f"          hwid: {port.hwid}")
        print("[anything else] :   exit...")
        selected = input("Your cohoice? ")
        if selected.isdigit() and int(selected) >= 0 and int(selected) < len(all_ports):
            port = all_ports[int(selected)]
            print(f"Using port for the ATMega-Z80 board: {port.device}\n")
            self.port = port.device
            self.vid = port.vid
            self.pid = port.pid
        else:    
            print("No port selected, bye!\n")
            sys.exit(1)
    

    def auto_detect(self, all_ports):
        boards = list(filter(lambda p: p.vid == USB_VID and p.pid == USB_PID, all_ports))
        if len(boards) == 1:
            p = boards[0]
            print(f"Single ATMega-Z80 board found on port: {p.device}\n" )
            self.port = p.device
            self.vid = p.vid
            self.pid = p.pid
            return True
        print("Unable to auto-detect the port for the ATMega-Z80 board. Please select:\n")
        return False
