
ATMega-Z80
==========

We are building here yet another 8bit hobby computer. Because it is fun. And this is what an engineer does, *"MIDWAY upon the journey of our life"*. ;)

Disclaimer: **Use anything in this repo with caution!**

Nothing works yet, we just started to plan the whole thing. The only thing we are sure is that things will change, as we move forward. Plus the fact that we will move forward very slooowly, as this is only a hobby project and everyone is quite busy with other things. 


Table of Contents
-----------------

  * [Goals](#goals)
  * [Hardware design](#hardware-design)
    * [Schema](#schema)
    * [PCB](#pcb)
  * [Using the board](#using-the-board)
  * [Development](#development)
    * [Prerequisites for Linux](#prerequisites-for-linux)
    * [Prerequisites for Windows](#prerequisites-for-windows)
    * [Prerequisites for Mac](#prerequisites-for-mac)
    * [(Optional) Configuring USB HWID](#optional-configuring-usb-hwid)
	* [ATMega Programmer](#atmega-programmer)
	  * [Initializing ATMega clock](#initializing-atmega-clock)
	  * [Deploying the ATMega code](#deploying-the-atmega-code)
	* [Deploying the Z80 'ROM' code](#deploying-the-z80-rom-code)
  * [License](#license)


Goals
=====

**The main goals are:**

 - Using a well-known 8bit CPU.
 - Let's keep the HW design relatively simple. (I'm personally more interested in the 8bit programming part and want to go deeper into the kernel and user code.)
 - We should be able to compile C code to run it on the new computer.
 - We don't need a full-fledged, stand-alone computer. I'm sure everyone has already a few of those on our desks... We don't build this to actually use it for work or gaming.
 - The whole computer should not have more than a handful of ICs. No FPGA or complex MCU with a need for an expensive programmer. Anyone should be able to build it on a breadboard for $40-50 worth of components (incl. programmer). 
 - Don't use logic level (voltage) converters, keep everything e.g. on 5V or on 3.3V.
 - We can have VGA / keyboard / SD Card support later (if someone needs it), but the first thing is to have USB support and connection to a PC, so we can implement a small thin terminal program that can send keyboard and mouse events, display video, play music even.

Hardware design
===============

The basic design is not new or original, it is very much based on other projects available on the internet. I'm especially grateful for [this one, the MBC2 project](https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer) (80% of the HW design comes from this project). 

 - We use a relatively new Z80 CPU, capable to run at 10MHz.
 - A 20MHz ATMega chip (similar to those you can find in Arduino boards) is helping the CPU as ROM emulator, boot loader and clock generator. Also handling all the IO requests from the CPU.
 - An IO wait logic is implemented using an SR latch, forcing the CPU to wait until the ATMega chip processed the IO operations.
 - The ATMgega chip can access both the data bus and the first 4 bits of the address bus. Having access to a part of the address bus makes the boot loading easier and also enables us to use IO and interrupt addresses.
 - For now, we added 512 KB RAM, using 16 KB banks. The CPU will be able to set the bank ID with an IO write operation and then the given 16 KB bank will be visible for the CPU on the third 16KB memory slot of the 64KB address space (between 0x8000 and 0xbfff).
 - We store the bank address in an 8 bit gated D latch IC. This means we can address the banks in 8 bits, so the design works until 4 MB (16KB * 256). But 4MB sounds a bit overkill and also more expensive. It is hard to find a single DIP parallel 8bit SRAM chip over 512 KB. Moving away from DIP makes it harder to assemble the computer on a breadboard, while using many separate 512 KB RAM chips would require a bit more complex banking logic (with lot more footprint and wiring on the breadboard). But in theory we can extend the RAM later easily if needed.
 - We have an USB controller chip, converting the UART port of ATMega to USB.
 - The USB port is used also for powering the whole atmega-z80.
 - We have a user button and LED, both can be used by the programmer to interact directly with the user. (who needs 101 keys and zillion pixels... :p)
 - We plan to add some status indicator LEDs:
	 - Power LED (power come through the USB cable)
	 - Z80 running LED (means the Z80 is active, not in 'halt' state)
	 - USB IO LED (ATMega is communicating through the USB)
	 - CPU IO LED (the CPU is doing IO read or write)
	 - Memory LED (the CPU is reading from / writing to anywhere in the RAM)
	 - Extended memory LED (the CPU is reading from / writing to the banked part of the RAM)
	 - User LED (some program running on the Z80 turned the user LED on)


Schema
------
Please take a look here for the latest version: [schema/atmega-z80.pdf](schema/atmega-z80.pdf)

You can find the editable files in the ['schema' folder](schema/). We use the KICad tool for schema design: https://www.kicad.org/download/


PCB
---

TODO


Using the board
===============

For now, you can only interact with the board using the console application. Later we might add keyboard and display support as well as other periferias. But the console app should be always capable to emulate these, as people might not want to use dedicated periferias for ATMega-Z80.

Prerequisites to use the console:
```
pip3 install pyserial==3.5
```

Using the console:
```
src/console/console.py
```

 
Development
===========

The source code needs to be deployed into the ATMega chip can be found here: ['src/atmega' folder](src/atmega/)

The source code running on the Z80 can be found here: ['src/z80' folder](src/z80/)

The source code for the console app (running on the PC and connecting to the ATMega on USB): ['src/console' folder](src/z80/)

The console is mainly developed for end users, to interact with the ATMega (until we don't have keyboard and display support, or if the user don't want to buy/attch separate devices to the board). However, during development you can also use the console as a debugger and you can also upload Z80 code using the console. However, to deploy code running on the ATMega, you need to use a programmer (see: [here](#atmega-programmer) )

You can not use the console and the ATMega programmer in the same time. **Make sure you are disconnecting the USB port while you are using the programmer!**



Prerequisites for Linux
-----------------------

TODO


Prerequisites for Windows
-----------------------

TODO


Prerequisites for Mac
---------------------

First, make sure you have xcode command line developer tools installed with:
```
$ xcode-select --install
```

Then install the latest versions of the avr command line tools (avr-libc included in avr-gcc):
```
$ brew tap osx-cross/avr
$ brew install avr-gcc@11 avr-binutils avrdude
```
(you can check the current versions of avr-gcc available through homebrew here: https://github.com/osx-cross/homebrew-avr)

Add the following two lines to your .bash_profile or .zshrc file:
```
export PATH="/usr/local/opt/avr-gcc@11/bin:$PATH"
export AVRGCC_LDFLAGS="-L/usr/local/opt/avr-gcc@11/lib"
```


(Optional) Configuring USB HWID
-------------------------------

You can program the MCP2221A chip, setting the HWID (Vendor ID: 0x1209, Product ID: 0x80A0) of the USB device. We 'registered' an open USB HWID in [pid.codes](https://pid.codes/). This is not mandatory, you can also use the MCP2221A chip with factory default HWID, but in this case the console won't auto-detect the port and you need to select the correct port manually from a list each time you start the console.

**Prerequisites (python):**

```
pip3 install mcp2221==1.0.0
pip3 install hidapi==0.10.1
```

**Prerequisites (C lib):**

The mcp2221 python library is using the hidapi library, which got merged into libusb in 2019. So depending on the age of your OS, you might need to install libhidapi or libusb. 

For me on Mac with up-to-date homebrew, this worked: `brew install hidapi`

According to the project page (https://github.com/trezor/cython-hidapi) for linux this one should work at the moment:
`sudo apt-get install python-dev libusb-1.0-0-dev libudev-dev`


**Setting the HWID for the board**

Use the following command to set the Vendor ID, Product ID and Product Description on the board: `./src/console/program_hwid.py`

**Reseting the HWID**

Use the following command to reset the Vendor ID, Product ID and Product Description on the board to the MPC2221A factory defaults:  `./src/console/program_hwid.py --reset`



ATMega Programmer
-----------------

To update the code running on the ATMega, we are using the open source UABAsp programmer (https://www.fischl.de/usbasp/). It comes with an ISP 10 connector. You need to connect the programmer to the ATMega ports indicated in the schema (RESET and PB3, PB6, PB7). Also you need to connect the 5V and GND from the programmer to the board, basically using the Programmer as a power source while you are programing the board. You can not use the console and the ATMega programmer in the same time. **Make sure you are disconnecting the USB port while you are using the programmer!**

The following photo shows how to connect the ISP 10 connector to your breadboard:

![](docs/ISP-10-pin.jpg?raw=true "USBAsp programmer connector")
 

### Initializing ATMega clock

ATMega shipped with internal 8MHz clock enabled and the clock prescaler set to 8, resulting in 1 MHz clock speed. In the code we assume you wired the 20 MHz crystal to ATMega already. To configure the external crystal oscillator to be used, you need to set the fuses on ATMega once before programming it. (useful online fuse bit calculator: http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega324pa)

**Make sure you are disconnecting the USB port while you are using the programmer to change the ATMega clock speed!**

Use the following command (from the `src/atmega` folder) to set the fuses to the full 20MHz speed: `make atmega_fuse_init`

If you want to rollback the factory defaults (1MHz internal clock), use: `make atmega_fuse_factory_reset`


### Deploying the ATMega code

Below it is shown how can you build the atmega code and flash it to ATMega. Note, the ATMega flash will not contain the Z80 'ROM code'. The Z80 code can be copied to ATMega by using the console app. **Make sure you are disconnecting the USB port while you are using the programmer!**
```
cd src/atmega
make
make program
```


Deploying the Z80 ROM code
--------------------------

The memory will be initialized on startup by the ATMega chip, copying the Z80 'ROM' into the memory from the ATMega flash. To update Z80 ROM, you don't need an AVR Programmer, you can use the console to uppload the new ROM file to ATMega.

```
TODO
```


License
=======
This is an open project (licensed under BSD-3). Use it for free, just don't point fingers / blame us ;)