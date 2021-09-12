
# atmega-z80


## Status
**Use anything in this repo with caution!**

Nothing works yet, we just started to plan the whole thing. The only thing we are sure is that things will change, as we move forward. Plus the fact that we will move forward very slooowly, as this is only a hobby project and everyone is quite busy with other things. 

## Goals
We are building here yet another 8bit hobby computer. Because it is fun. And this is what an engineer does, when *"MIDWAY upon the journey of our life we found ourself within a forest dark"*. ;)

**The main goals are:**

 - Using a well-known 8bit CPU.
 - Let's keep the HW design relatively simple. (I'm personally more interested in the 8bit programming part and want to go deeper into the kernel and user code.)
 - We should be able to compile C code to run it on the new computer.
 - We don't need a full-fledged, stand-alone computer. I'm sure everyone has already a few of those on our desks... We don't build this to actually use it for work or gaming.
 - The whole computer should not have more than a handful of ICs. No FPGA or complex MCU with a need for an expensive programmer. Anyone should be able to build it on a breadboard for $20-30 worth of components. 
 - Don't use logic level (voltage) converters, keep everything e.g. on 5V or on 3.3V.
 - We can have VGA / keyboard / SD Card support later (if someone needs it), but the first thing is to have USB support and connection to a PC, so we can implement a small thin terminal program that can send keyboard and mouse events, display video, play music even.

## Hardware design plans
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
	 - USB IO LED (ATMega is sending data through the USB)
	 - CPU IO LED (the CPU is doing IO read or write)
	 - Memory LED (the CPU is reading from / writing to anywhere in the RAM)
	 - Extended memory LED (the CPU is reading from / writing to the banked part of the RAM)
	 - User LED (some program running on the Z80 turned the user LED on)


## Schema
Please take a look here for the latest version: [schema/atmega-z80.pdf](schema/atmega-z80.pdf)

You can find the editable files in the ['schema' folder](schema/). We use the KICad tool for schema design: https://www.kicad.org/download/

 
## Connecting the programmer

We are using the open source UABAsp programmer (https://www.fischl.de/usbasp/). It comes with an ISP 10 connector. You need to connect the programmer to the ATMega ports indicated in the schema (RESET and PB3, PB6, PB7). Also you need to connect the 5V and GND from the programmer to the board, basically using the Programmer as a power source while you are programing the board.

The following photo shows how to connect the ISP 10 connector to your breadboard:

![](docs/ISP-10-pin.jpg?raw=true "USBAsp programmer connector")
 
## Development

The source code needs to be deployed into the ATMega chip can be found here: ['src/atmega' folder](src/atmega/)

The source code running on the Z80 can be found here: ['src/z80' folder](src/z80/)

### Prerequisites for Mac

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

### Initializing ATMega clock

ATMega shipped with internal 8MHz clock enabled and the clock prescaler set to 8, resulting in 1 MHz clock speed. In the code we assume you wired the 20 MHz crystal to ATMega already. To configure the external crystal oscillator to be used, you need to set the fuses on ATMega once before programming it. (useful online fuse bit calculator: http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega324pa)

Use the following command (from the `src/atmega` folder) to set the fuses to the full 20MHz speed: `make atmega_fuse_init`

If you want to rollback the factory defaults (1MHz internal clock), use: `make atmega_fuse_factory_reset`


### Building the and deploying the code to the board

First build the Z80 code:
```
cd src/z80

# TODO
```

Then build the atmega code and flash ATMega. Note, the ATMega flash will also contain the Z80 code, which will be copied into the Z80 RAM during startup.
```
cd src/atmega
make
make program
```

## License
This is an open project (licensed under BSD-3). Use it for free, just don't point fingers / blame us ;)