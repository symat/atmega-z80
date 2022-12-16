#!/usr/bin/env python3

from usbconnector import ATMegaZ80UsbConnector
import argparse, functools, operator, serial, struct, sys, time

MAX_PROG_SIZE = 0x10000 - 256

BAUD_RATE = 9600
CHUNK_SIZE = 256

CMD_LOAD_Z80_BINARY_FROM_USART = 1
CMD_ECHO = 2

ERR_OK = 0
ERR_TIMEOUT = 1
ERR_INVALID_SIZE = 2
ERR_CHECKSUM_FAILED = 3

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
        print('a')
        # Send command code
        #packet = bytearray()
        #packet.append(0x01)
        #p.write(packet)

        p.write(struct.pack('B', CMD_LOAD_Z80_BINARY_FROM_USART))
        print('b')

        time.sleep(1)

        # Send program length as a 16bit unsigned value in network byte order (big-endian)
        p.write(struct.pack('!H', z80_prog_length))
        print('c')

        # Wait for ACK
        err = p.read()[0]
        print(f'd : {err}')
        if err != ERR_OK:
            print(f'The board has refused the upload (error code = {err})! Exiting...')
            return
        print('e')

        block_count = p.read()[0]
        print(f'f : {block_count}')
        exit();

        # Send program in CHUNK_SIZE sized chunks
        pos = 0
        while pos < z80_prog_length:
            chunk_size = min(CHUNK_SIZE, z80_prog_length - pos)
            prog_range = z80_prog[pos : pos + chunk_size]
            chksum = functools.reduce(operator.xor, prog_range, 0)

            # chunk_size sent is always one less to allow 256 byte sized chunks
            p.write(struct.pack('B', chunk_size - 1))

            # Wait for ACK
            err = p.read()[0]
            if err != ERR_OK:
                print(f'The board has refused the next chunk (error code = {err})! Exiting...')
                return
                
            p.write(prog_range)
            p.write(struct.pack('b', chksum))

            # Wait for ACK
            err = p.read()[0]
            if err != ERR_OK:
                print(f'Transfer error (error code = {err})! Exiting...')
                return

            pos += chunk_size

        # Read console output from Z80
        while True:
            ch = p.read()[0]
            # End of transmission check
            if ch != 0x04:
                sys.stdout.write(chr(ch))
            else:
                break

main()
