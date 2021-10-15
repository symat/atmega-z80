#!/usr/bin/env python3

from usbconnector import PortDetector
from usbconnector import USB_VID
from usbconnector import USB_PID
from usbconnector import USB_DESC

import mcp2221, struct, sys

def generate_new_chip_settings(original, new_vid, new_pid):
    new_settings = original.copy()
    new_settings[4 : 8] = struct.pack("<HH", new_vid, new_pid)
    return new_settings

port_detector = PortDetector()

print(f"Connecting to device: VID={port_detector.vid:#0{6}x}   PID={port_detector.pid:#0{6}x}")

mcp2221_dev = mcp2221.Mcp2221(vid = port_detector.vid, pid = port_detector.pid)

print(f"\n==== The current settings in the device:")
product_description = mcp2221_dev.ReadProductDescription()
print(f"Description: {product_description}")

chip_settings = mcp2221_dev.ReadChipSettings()
print(f"Current chip setting bytes: {chip_settings}\n")

if len(sys.argv) == 2 and sys.argv[1] == "--reset":
    print(f"==== Resetting the MCP2221 device to factory defaults on port: {port_detector.port}")
    new_chip_settings = generate_new_chip_settings(chip_settings, mcp2221.DEFAULT_USB_VID, mcp2221.DEFAULT_USB_PID)
    new_product_description = mcp2221.DEFAULT_PRODUCT_DESCRIPTION
    print(f"New Chip Settings to write: {new_chip_settings}")
    print(f"New Description to write: {new_product_description}")

else:
    print(f"==== Initializing ATMega-Z80 USB Hardware ID on the board: {port_detector.port}")
    new_chip_settings = generate_new_chip_settings(chip_settings, USB_VID, USB_PID)
    new_product_description = USB_DESC
    print(f"New Chip Settings to write: {new_chip_settings}")
    print(f"New Description to write: {new_product_description}")

if input("\nAre you sure you want to continue? (type 'yes') ") != "yes":
    print("\nAborting....")
    sys.exit(0)

mcp2221_dev.WriteProductDescription(new_product_description)
mcp2221_dev.WriteChipSettings(new_chip_settings)

print("\nDone... Please reset the board for the changes to take place")



# factory defaults:
# [77, 0, 67, 0, 80, 0, 50, 0, 50, 0, 50, 0, 49, 0, 32, 0, 85, 0, 83, 0, 66, 0, 45, 0, 73, 0, 50, 0, 67, 0, 47, 0, 85, 0, 65, 0, 82, 0, 84, 0, 32, 0, 67, 0, 111, 0, 109, 0, 98, 0, 111, 0]
#          desc: MCP2221 USB-I2C/UART Combo
#          hwid: USB VID:PID=04D8:00DD LOCATION=20-2.4.3
# 4 216 : 0 221