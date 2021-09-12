# expecting the AVRGCC_LDFLAGS set to your avr-gcc/lib folder

MCU=atmega324pa
PROGRAMMER=usbasp
CFLAGS=-g -Wall -mcall-prologues -mmcu=$(MCU) -Os
LDFLAGS=-Wl,-gc-sections -Wl,-relax $(AVRGCC_LDFLAGS)
CC=avr-gcc
TARGET=main

all: $(TARGET).hex

clean:
	rm -f *.o *.elf *.hex

%.hex: %.elf
	avr-objcopy -R .eeprom -O ihex $< $@

$(TARGET).elf: $(TARGET).o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

program: $(TARGET).hex
	avrdude -c $(PROGRAMMER) -p $(MCU) -U flash:w:$<:i

atmega_fuse_init:
	# 20 MHz external crystal oscillator
	avrdude -c $(PROGRAMMER) -p $(MCU) -U lfuse:w:0xF7:m
	avrdude -c $(PROGRAMMER) -p $(MCU) -U hfuse:w:0x99:m
	avrdude -c $(PROGRAMMER) -p $(MCU) -U efuse:w:0xFF:m
	avrdude -c $(PROGRAMMER) -p $(MCU) -U lock:w:0xFF:m

atmega_fuse_factory_reset:
	# 1 MHz internal clock, no locks
	avrdude -c $(PROGRAMMER) -p $(MCU) -U lfuse:w:0x62:m
	avrdude -c $(PROGRAMMER) -p $(MCU) -U hfuse:w:0x99:m
	avrdude -c $(PROGRAMMER) -p $(MCU) -U efuse:w:0xFF:m
	avrdude -c $(PROGRAMMER) -p $(MCU) -U lock:w:0xFF:m
	