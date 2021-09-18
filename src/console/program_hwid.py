#!/usr/bin/env python3

from usbconnector import PortDetector
from usbconnector import USB_VID
from usbconnector import USB_PID
from usbconnector import USB_DESC

from MCP2221 import MCP2221
import sys

FACTORY_DEF_USB_VID = int("0x04D8", base=16)
FACTORY_DEF_USB_PID = int("0x00DD", base=16)
FACTORY_DEF_USB_DESC = "MCP2221 USB-I2C/UART Combo"


def desc_str_to_ints(desc_str):
    # in position 0 we have the number of bytes to follow (2+2*<number of characters>
    # in position 1 we have 0x03
    # then we have [lower byte, upper byte] pairs for each 16bit unicodse character
    # in the end, we have a tailing 0    
    length = 2 + 2*len(desc_str)
    result = [length, 3]
    for chr in desc_str:
        bytes = ord(chr).to_bytes(2, byteorder='big')
        result.append(bytes[1])
        result.append(bytes[0])
    return result

def desc_ints_to_str(desc_int_array):
    result = ""
    # in position 0 we have a fixed 0x03
    # then we have [lower byte, upper byte] pairs for each 16bit unicodse character
    # in the end, we have a tailing 0
    for i in range(0, int((len(desc_int_array) - 1) / 2)):
        lower_byte = desc_int_array[1 + 2*i]
        higher_byte = desc_int_array[2 + 2*i]
        unicode_char = chr(higher_byte*256 + lower_byte)
        result += unicode_char
    return result

def generate_new_chip_settings(original, new_vid, new_pid):
    new_settings = original.copy()
    new_vid_bytes = new_vid.to_bytes(2, byteorder='big')
    new_pid_bytes = new_pid.to_bytes(2, byteorder='big')
    new_settings[4] = int(new_vid_bytes[1])
    new_settings[5] = int(new_vid_bytes[0])
    new_settings[6] = int(new_pid_bytes[1])
    new_settings[7] = int(new_pid_bytes[0])
    return new_settings


port_detector = PortDetector()

print(f"Connecting to device: VID={port_detector.vid:#0{6}x}   PID={port_detector.pid:#0{6}x}")

mcp2221 = MCP2221.MCP2221(VID=port_detector.vid, PID=port_detector.pid)

print()
print(f"==== The current settings in the device:")

desc_int_array = mcp2221.ReadFlash(MCP2221.FLASH.USB_PRODUCT_DESCRIPTOR)

print(f"Description: {desc_ints_to_str(desc_int_array)}")
print(f"Current description bytes (in read format): {str(desc_int_array)}")
print(f"Current description bytes (in write format): {desc_str_to_ints(desc_ints_to_str(desc_int_array))}")

serial_int_array = mcp2221.ReadFlash(MCP2221.FLASH.CHIP_SETTING)
print(f"Current chip setting bytes: {str(serial_int_array)}")
print()

if len(sys.argv) == 2 and sys.argv[1] == "--reset":
    print(f"==== Resetting the MCP2221 device to factory defaults on port: {port_detector.port}")
    new_chip_settings=generate_new_chip_settings(serial_int_array, FACTORY_DEF_USB_VID, FACTORY_DEF_USB_PID)
    print(f"New Chip Settings to write: {new_chip_settings}")
    print(f"New Description to write: {FACTORY_DEF_USB_DESC}")
    new_desc_array = desc_str_to_ints(FACTORY_DEF_USB_DESC)
    print(f"New Description bytes to write: {new_desc_array}")

else:
    print(f"==== Initializing ATMega-Z80 USB Hardware ID on the board: {port_detector.port}")
    new_chip_settings=generate_new_chip_settings(serial_int_array, USB_VID, USB_PID)
    print(f"New Chip Settings to write: {new_chip_settings}")
    print(f"New Description to write: {USB_DESC}")
    new_desc_array = desc_str_to_ints(USB_DESC)
    print(f"New Description bytes to write: {new_desc_array}")

print()
if input("Are you sure you want to continue? (type 'yes') ") != "yes":
    print()
    print("Aborting....")
    sys.exit(0)

mcp2221.WriteFlash(MCP2221.FLASH.USB_PRODUCT_DESCRIPTOR, new_desc_array)
mcp2221.WriteFlash(MCP2221.FLASH.CHIP_SETTING, new_chip_settings)

print()
print("Done... Please reset the board for the changes to take place")



# factory defaults:
# [3, 77, 0, 67, 0, 80, 0, 50, 0, 50, 0, 50, 0, 49, 0, 32, 0, 85, 0, 83, 0, 66, 0, 45, 0, 73, 0, 50, 0, 67, 0, 47, 0, 85, 0, 65, 0, 82, 0, 84, 0, 32, 0, 67, 0, 111, 0, 109, 0, 98, 0, 111, 0, 0]
#           desc: MCP2221 USB-I2C/UART Combo
#          hwid: USB VID:PID=04D8:00DD LOCATION=20-2.4.3
# 4 216 : 0 221