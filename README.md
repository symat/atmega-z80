
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

You can set the fuses in ATMega to various clock speeds. We support the following speeds:
- 20MHz (the full speed of our external osciallator)
- 8 MHz (internal RC Oscillator)
- 2.5 MHz (dividing the external oscillator speed by 8)
- 1 MHz (internal RC Oscillator, divided by 8; this is the factory default for the ATMega chip)
- 128 KHz (using the internal ATMega slow oscillator - might be tricky, see the slow clock warning)
- 16 KHz (internal slow oscillator, divided by 8 - might be tricky, see the slow clock warning)

When you change the clock speed, you also need to change the MCU_SPEED parameter, to instruct the copmiler to use the proper speed. You need to run `cmake` and `make clean` and `make` to make sure your changes are taking effect. To test your settings, you can use the `make upload_blinky` to upload a program to ATMega which will blink the user led, once in each second. You can see the exact steps below.

```
# if you want to change the clock speed to: 20MHz
make atmega_fuse_init
cmake -DMCU_SPEED=20000000 .

# if you want to change the clock speed to: 8MHz
make atmega_fuse_init_8000khz
cmake -DMCU_SPEED=8000000 .

# if you want to change the clock speed to: 2.5MHz
make atmega_fuse_init_2500khz
cmake -DAVRDUDE_BIT_CLOCK=3 -DMCU_SPEED=2500000 .

# if you want to change the clock speed to: 1MHz (this is the factory default)
make atmega_fuse_factory_reset
cmake -DAVRDUDE_BIT_CLOCK=8 -DMCU_SPEED=1000000 .

# if you want to change the clock speed to: 128KHz (this can be a bit tricky, see the SLOW CLOCK WARNING above)
make atmega_fuse_init_128khz
cmake -DAVRDUDE_BIT_CLOCK=50 -DMCU_SPEED=128000 .

# if you want to change the clock speed to: 32KHz (this can be a bit tricky, see the SLOW CLOCK WARNING above)
make atmega_fuse_init_16khz
cmake -DAVRDUDE_BIT_CLOCK=200 -DMCU_SPEED=16000 .


# you need to rebuild the code with the new clock settings
make clean
make

# this command will make your user LED on the board to flash once in each second
# this way you can make sure the clock is set properly
# (you will have to re-deploy the original ATMega code after this step)
make upload_blinky
```


**!SLOW CLOCK WARNING!** (check this below, before you would try to use the slower clock options)

You might see the following error when you try to program the board, after you changed the clock speed to some slower option:
```
avrdude: error: program enable: target doesn't answer. 1 
avrdude: initialization failed, rc=-1
         Double check connections and try again, or use -F to override
         this check.

avrdude done.  Thank you.
```
The problem can be that the usbasp programmer is trying to talk 'too fast' with the ATMega chip on our board, if you set slow ATMega clocks. Normally you can use the `-B` option for avrdude to instruct the programmer to talk slower. This option can be provided through the AVRDUDE_BIT_CLOCK cmake parameter. I tested the various clock speeds with different AVRDUDE_BIT_CLOCK parameters, so the above examples should work. However, there is a chance that your programmer is different. If you see this error, then you can try to increase the bit clock for your programmer. Also, increasing the bit clock will make the programming slower. So you might want to find the ideal AVRDUDE_BIT_CLOCK for your programmer.

An other 'slow clock' problem you can face with usbasp programmer is that avrdude actually is unable to set the bit clock:
```
avrdude: warning: cannot set sck period. please check for usbasp firmware update.
```
Based on some googling, this happens when you have a very old usbasp programmer or if your programmer has some non-official usbasp frameware. In this case you either need to upgrade the programmer's firmware or you need to connect the 'slow clock' jumper on your programmer. This is JP1 on the original schematic of usbasp (https://www.fischl.de/usbasp/), while in case of my programmer, this was actually marked as JP3 (so be careful). If you choose the other option, the latest frameware can be downloaded from the usbasp site. However you will need to have a second programmer to be able to refresh the framework of your original programmer. (but I had to do this, as in my case, even the 'slow clock' jumper was not enough, after I set my ATMega to 16KHz)

Anyway, if you don't want to spend time on these 'slow clock' problems, then you can simply skip using clock rates slower than 1 MHz.


### Deploying the ATMega code

Below it is shown how can you build the atmega code and flash it to ATMega. Note, the ATMega flash will not contain the Z80 'ROM code'. The Z80 code can be copied to ATMega by using the console app. **Make sure you are disconnecting the USB port while you are using the programmer!**
```
make clean
make
make upload_z80_prog_loader
```


Deploying the Z80 ROM code
--------------------------

The memory will be initialized on startup by the ATMega chip, copying the Z80 'ROM' into the memory from the ATMega flash. To update Z80 ROM, you don't need an AVR Programmer, you can use the console to uppload the new ROM file to ATMega.

```
TODO
```


License
=======
This is an open project. The software is licensed under BSD-3, while the hardware is licensed under CERN-OHL-P. Both licenses are permissive. Feel free to use anything here, just don't point fingers / blame us ;)