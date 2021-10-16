import hid, itertools, struct

DEFAULT_USB_VID = 0x04D8
DEFAULT_USB_PID = 0x00DD
DEFAULT_PRODUCT_DESCRIPTION = "MCP2221 USB-I2C/UART Combo"

# HID report size
BUF_LENGTH = 64

class Error(Exception):
    def __init__(self, message):
        self.message = message

class Mcp2221:
    def __init__(self, vid = DEFAULT_USB_VID, pid = DEFAULT_USB_PID, dev = 0):
        self.vid = vid
        self.pid = pid
        self.hid_dev = hid.device()
        self.hid_dev.open_path(hid.enumerate(vid, pid)[dev]["path"])

    def ReadProductDescription(self):
        # See MCP2221 datasheet page 26 (table 3-3) for the description of fields
        response = self._HidSend(0xB0, 0x03)
        # See MCP2221 datasheet page 30 (table 3-8) for response fields
        if len(response) < 4 or response[3] != 0x03:
            raise Error("Unexpected response from chip")

        str_length = response[2] - 2
        return self._StrDecode(response[4 : 4 + str_length])

    def WriteProductDescription(self, descr):
        enc_descr = self._StrEncode(descr)
        enc_descr_length = len(enc_descr)
        if enc_descr_length > BUF_LENGTH - 4:
            raise Error("Description string is too long")

        # See MCP2221 datasheet page 36 (table 3-15) for the description of fields
        self._HidSend(0xB1, 0x03, enc_descr_length + 2, 0x03, *enc_descr)

    def ReadChipSettings(self):
        # See MCP2221 datasheet page 26 (table 3-3) for the description of fields
        response = self._HidSend(0xB0, 0x00)
        # See MCP2221 datasheet page 27 (table 3-5) for response fields
        settings_length = response[2]
        return response[4 : 4 + settings_length]

    def WriteChipSettings(self, new_chip_settings):
        # See MCP2221 datasheet page 33 (table 3-12) for the description of fields
        self._HidSend(0xB1, 0x00, *new_chip_settings)

    def _HidSend(self, *args):
        args_length = len(args)
        if args_length == 0:
            raise Error("Send buffer is empty")
        elif args_length > BUF_LENGTH:
            raise Error("Send buffer is too long")

        command_code = args[0]
        padding = [0x00] * (BUF_LENGTH - args_length)
        # First 0x00 means there are no report IDs present
        # See https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/hidsdi/nf-hidsdi-hidd_getinputreport#remarks
        self.hid_dev.write([0x00, *args, *padding])

        response = self.hid_dev.read(BUF_LENGTH)
        if len(response) < 2:
            raise Error("Chip response is too short")
        elif response[0] != command_code:
            raise Error(f"Invalid command code in response: got {response[0]:#02x} expected {command_code:#02x}")
        elif response[1] != 0x00:
            raise Error("Command failed")

        return response

    @staticmethod
    def _StrEncode(str):
        # Converts 'str' to a sequence of 16-bits unicode code points
        # then encodes it as a list of bytes in little-endian byte order
        # See MCP2221 datasheet page 30 (table 3-8)
        return list(itertools.chain.from_iterable([struct.pack('<H', ord(ch)) for ch in str]))

    @staticmethod
    def _StrDecode(u16str):
        # Converts a list of bytes to 16-bits unicode code points then concatenates them into a string
        # See MCP2221 datasheet page 30 (table 3-8)
        return "".join([ chr(ucode[0]) for ucode in struct.iter_unpack("<H", bytes(u16str)) ])
