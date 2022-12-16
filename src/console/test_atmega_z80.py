from usbconnector import ATMegaZ80UsbConnector
import serial, argparse, struct, unittest, time

MAX_PROG_SIZE = 0x10000 - 256

BAUD_RATE = 9600
CHUNK_SIZE = 256

CMD_LOAD_Z80_BINARY_FROM_USART = 1
CMD_ECHO = 2
CMD_UPLOAD_CODE_TO_RAM = 3
CMD_DOWNLOAD_FIRST_16_BYTE_FROM_RAM = 4

ERR_OK = 0
ERR_TIMEOUT = 1
ERR_INVALID_SIZE = 2
ERR_CHECKSUM_FAILED = 3
ERR_UNKNOWN_COMMAND = 4

class ATMegaZ80Tests(unittest.TestCase):
    BOARD = None

    def send_u8(self, value):
        self.BOARD.write(struct.pack('B', value))

    def send_noops(self, num):
        # the NOOP operation has opcode 0x00
        for i in range(num): 
            self.send_u8(0)

    def send_inc_bytes(self, num):
         for i in range(num): 
            self.send_u8(i)

    def send_u16(self, value):
        self.BOARD.write(struct.pack('!H', value))

    def assert_response(self, expected):
        err = p.read()[0]
        msg = f"wrong response from ATMega. expected: {expected}, actual: {err}"
        self.assertEqual(err, expected, msg)

    def assert_inc_bytes(self, num):
         for i in range(num): 
            self.assert_response(i)


    # executing some echo commands with various random bytes
    def test_connection_with_atmega(self):
        self.send_u8(CMD_ECHO)
        self.send_u8(67)
        self.assert_response(67)
        self.send_u8(CMD_ECHO)
        self.send_u8(5)
        self.assert_response(5)
        self.send_u8(CMD_ECHO)
        self.send_u8(163)
        self.assert_response(163)
 

    def test_timeout_before_program_upload(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        # next the board waits for the program length, but let's force a timeout here 
        # (the program length is only 2 bytes, expected to be sent very quickly)
        time.sleep(0.1)
        self.assert_response(ERR_TIMEOUT)

        # now let's assume we sent the program length anyway (not detecting the timeout yet)
        # this should result in two ERR_UNKNOWN_COMMANDs
        self.send_u16(100) # 100 in big endian:  0x00 0x64
        self.assert_response(ERR_UNKNOWN_COMMAND)
        self.assert_response(ERR_UNKNOWN_COMMAND)

        # after the ERR_UNKNOWN_COMMAND, the board still should be functional
        # (the input buffer should be flushed, the board should be ready for a new command)
        self.send_u8(CMD_ECHO)
        self.send_u8(163)
        self.assert_response(163)


    def test_block_count_calculated_before_program_upload(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        self.send_u16(10000) # we plan to upload 10000 bytes in 40 blocks (39 full blocks plus 16 bytes)
        self.assert_response(ERR_OK)
        self.assert_response(40) # the board is expecting 40 blocks
        time.sleep(0.5) # do a timeout so that the next test can start with a fresh state
        self.assert_response(ERR_TIMEOUT)


    def test_upload_one_block_of_noop_program(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        self.send_u16(10) # we plan to upload 10 bytes
        self.assert_response(ERR_OK)
        self.assert_response(1) # the board is expecting 1 block
        self.send_u8(9) # sending chunk size 9 (meaning 10 bytes)
        self.send_noops(10) # sending 10 bytes of Z80 NOOP operations
        self.send_u8(0) # checksum should be 0 (the XOR operation of the bytes)
        self.assert_response(ERR_OK) # block uploaded
   

    def test_upload_one_full_block_of_noop_program(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        self.send_u16(256) # we plan to upload 256 bytes
        self.assert_response(ERR_OK)
        self.assert_response(1) # the board is expecting 1 block
        self.send_u8(255) # sending chunk size 255 (meaning 256 bytes)
        self.send_noops(256) # sending 256 bytes of Z80 NOOP operations
        self.send_u8(0) # checksum should be 0 (the XOR operation of the bytes)
        self.assert_response(ERR_OK) # block uploaded
 

    def test_upload_two_blocks_of_noop_program(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        self.send_u16(356) # we plan to upload 356 bytes (one full 256 bytes long block, plus one 100 bytes long)
        self.assert_response(ERR_OK)
        self.assert_response(2) # the board is expecting 1 block
        # block 1:
        self.send_u8(255) # sending chunk size 255 (meaning 256 bytes)
        self.send_noops(256) # sending 256 bytes of Z80 NOOP operations
        self.send_u8(0) # checksum should be 0 (the XOR operation of the bytes)
        self.assert_response(ERR_OK) # block uploaded
        # block 2:
        self.send_u8(99) # sending chunk size 99 (meaning 100 bytes)
        self.send_noops(100) # sending 100 bytes of Z80 NOOP operations
        self.send_u8(0) # checksum should be 0 (the XOR operation of the bytes)
        self.assert_response(ERR_OK) # block uploaded
 

    def test_invalid_block_size_should_be_noticed(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        self.send_u16(10) # we plan to upload 10 bytes
        self.assert_response(ERR_OK)
        self.assert_response(1) # the board is expecting 1 block
        self.send_u8(10) # sending chunk size 10 (meaning 11 bytes)
        self.send_noops(11) # sending 11 bytes of Z80 NOOP operations
        self.send_u8(0) # checksum should be 0 (the XOR operation of the bytes)
        self.assert_response(ERR_INVALID_SIZE) # chunk size is OK


    def test_invalid_checksum_should_be_noticed(self):
        self.send_u8(CMD_LOAD_Z80_BINARY_FROM_USART)
        self.send_u16(10) # we plan to upload 10 bytes
        self.assert_response(ERR_OK)
        self.assert_response(1) # the board is expecting 1 block
        self.send_u8(9) # sending chunk size 9 (meaning 10 bytes)
        self.send_noops(10) # sending 256 bytes of Z80 NOOP operations
        self.send_u8(123) # checksum should be 0 (the XOR operation of the bytes)
        self.assert_response(ERR_CHECKSUM_FAILED) # block uploaded


    def test_upload_z80_code_to_ram(self):
        self.send_u8(CMD_UPLOAD_CODE_TO_RAM)
        self.send_u8(11) # sending number of bytes: 10 is meaning 11 bytes
        self.send_inc_bytes(12) # sending 11 bytes (incrementally: 0, 1, 2, 3...)
        self.send_u8(0) # checksum should be 0 (the XOR of the byte values)
        self.assert_response(ERR_OK)


    def test_invalid_size_while_upload_z80_code_to_ram(self):
        self.send_u8(CMD_UPLOAD_CODE_TO_RAM)
        self.send_u8(16) # sending number of bytes: 16 is meaning 17 bytes (invalid: max 16 bytes allowed)
        self.assert_response(ERR_INVALID_SIZE)


    def test_invalid_checksum_while_upload_z80_code_to_ram(self):
        self.send_u8(CMD_UPLOAD_CODE_TO_RAM)
        self.send_u8(10) # sending number of bytes: 10 is meaning 11 bytes
        self.send_inc_bytes(11) # sending 11 bytes (incrementally: 0, 1, 2, 3...)
        self.send_u8(123) # checksum should be 1 (the XOR of the byte values)
        self.assert_response(ERR_CHECKSUM_FAILED)

    def test_download_first_16_byte_of_ram(self):
        self.send_u8(CMD_UPLOAD_CODE_TO_RAM)
        self.send_u8(15) # sending number of bytes: 15 is meaning 16 bytes
        self.send_inc_bytes(16) # sending 16 bytes (incrementally: 0, 1, 2, 3...)
        self.send_u8(0) # checksum should be 0 (the XOR of the byte values)
        self.assert_response(ERR_OK)
        self.send_u8(CMD_DOWNLOAD_FIRST_16_BYTE_FROM_RAM)
        self.assert_response(15) # the board will send 16 bytes
        self.assert_inc_bytes(16) # assert receiving 16 bytes (incrementally: 0, 1, 2, 3...)
        self.assert_response(0) # checksum should be 0 (the XOR of the byte values)



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = 'ATMega-Z80 test tool')
    parser.add_argument('-p', '--port', help='serial port of the atmega-z80 (default: autodetect)')
    parser.add_argument('-t', '--test', help='prefix of test case names (default all tests: ATMegaZ80Tests.*)', default="ATMegaZ80Tests")
    args = parser.parse_args()
    usb_connector = ATMegaZ80UsbConnector(args.port)
    with serial.Serial(usb_connector.port, BAUD_RATE) as p:
        ATMegaZ80Tests.BOARD = p
        print("starting tests:\n----------------------------------------------------------------------\n")
        t = unittest.TestLoader().loadTestsFromName("__main__." + args.test)
        unittest.TextTestRunner(verbosity=2).run(t)
    
