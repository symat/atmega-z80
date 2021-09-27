#!/usr/bin/env python3

from usbconnector import ATMegaZ80UsbConnector
import argparse, functools, operator, serial, struct

MAX_PROG_SIZE = 0x10000 - 256

BAUD_RATE = 9600
CHUNK_SIZE = 256

def main():
    parser = argparse.ArgumentParser(description = 'Z80 program loader tool')
    parser.add_argument('prog', help='Z80 program file to load (binary format)')
    parser.add_argument('-p', '--port', help='serial port of the atmega-z80 (default: autodetect)')
    args = parser.parse_args()
    
    try:
        f = open(args.prog, 'rb')
        z80_prog = f.read()
        f.close()
        z80_prog_length = len(z80_prog)
        
        if z80_prog_length > MAX_PROG_SIZE:
            print(f'Z80 program file is too big! (max. {MAX_PROG_SIZE} bytes)')
            return
        
    except FileNotFoundError:
        print(f"Unable to open Z80 program file: '{args.prog}'")
        return
    
    usb_connector = ATMegaZ80UsbConnector(args.port)
    with serial.Serial(usb_connector.port, BAUD_RATE) as p:
        # Send command code
        p.write(struct.pack('b', 1))

        # Send program length as a 16bit unsigned value in network byte order (big-endian)
        p.write(struct.pack('!H', z80_prog_length))

        # Wait for ACK
        if p.read(1) != 1:
            print('The board has refused the upload! Exiting...')
            return

        # Send program in CHUNK_SIZE sized chunks
        pos = 0
        while pos < z80_prog_length:
            chunk_size = min(CHUNK_SIZE, z80_prog_length - pos)
            prog_range = chunk_size[pos : pos + chunk_size]
            chksum = functools.reduce(operator.xor, prog_range, 0)

            # chunk_size sent is always one less to allow 256 byte sized chunks
            p.write(struct.pack('!H', chunk_size - 1))
            p.write(prog_range)
            p.write(struct.pack('b', chksum))

            # Wait for ACK
            if p.read(1) != 1:
                print('Transfer error! Exiting...')
                return

            pos += chunk_size

main()
