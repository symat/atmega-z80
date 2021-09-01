
# atmega-z80


## Status
**Use anything in this repo with caution!**
Nothing works yet, we just started to plan the whole thing. The only thing we are sure is that things will change, as we move forward... And we will move forward very slooowly, as this is only a hobby project and everyone is quite busy with other things. 

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
 - Similarly to the [mbc2](https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer) project, an IO wait logic is implemented, forcing the CPU to wait until the ATMega chip processed the IO operations.
 - A bit different from the [mbc2](https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer) project, in our case the ATMgega chip can access both the data bus and the first 6 bit of the address bus. Having access to a larger part of the address bus makes the boot loading easier and also enables us to use IO and interrupt addresses (hopefully making the comunication between the CPU and ATMega easir and faster).
 - For now, we added 512 KB RAM, using 16KB banks. The CPU will be able to set the bank ID with an IO write operation and then the given 16 KB bank will be visible for the CPU on the third 16KB memory slot of the 64KB address space (starting from 0x8000).
 - We store the bank address in an 8 bit gated D-latch chip. This means we can address the banks in 8 bits, so the design works until 4 MB (16KB * 256). But 4MB sounds a bit overkill and also more expensive. It is hard to find a single DIP SRAM chip over 512 KB. Moving away from DIP makes it harder to assemble the computer on a breadboard, while using many separate 512 KB RAM chips would require more complex banking logic (with lot more footprint and wiring on the breadboard). But in theory we can extend the RAM later easily if needed.
 - We have an USB controller chip, converting the UART port of ATMega to USB.
 - The USB port is used also for powering the whole atmega-z80.
 - We have an user button and LED, can be used by the programmer to interact directly with the user. (who needs 101 keys and zillion pixels... :p)
 - We plan to add some status indicator LEDs:
	 - Power LED (power come through the USB cable)
	 - Z80 running LED (means the Z80 is active, not in 'halt' state)
	 - USB IO LED (ATMega is sending data through the USB)
	 - CPU IO LED (the CPU is doing IO read or write)
	 - Memory LED (the CPU is reading from / writing to anywhere in the RAM)
	 - Extended memory LED (the CPU is reading from / writing to the banked part of the RAM)
	 - User LED (some program running on the Z80 turned the user LED on)


## Schema
Please take a look here for the latest version: [schema/atmega-z80.pdf](schema/atmega-z80.pdf) or [schema/atmega-z80.jpg](schema/atmega-z80.jpg)
You can find the editable files in the ['schema' folder](schema/). We use the KICad tool for schema design: https://www.kicad.org/download/

## License
This is an open project (licensed under BSD-3). Use it for free, just don't point fingers / blame us ;)