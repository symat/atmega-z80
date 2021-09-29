#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define USART_BAUD_RATE 9600

#define PORT_DATA_BUS PORTA
#define DDR_DATA_BUS DDRA
#define PIN_DATA_BUS PINA

#define PORT_ADDRESS_BUS PORTB
#define DDR_ADDRESS_BUS DDRB
#define PIN_ADDRESS_BUS PINB

#define PORT_SIG1 PORTC
#define DDR_SIG1 DDRC
#define PIN_SIG1 PINC

#define PORT_SIG2 PORTD
#define DDR_SIG2 DDRD
#define PIN_SIG2 PIND

#define BUSRQ_SIG1 PC3
#define BUSACK_SIG1 PC4
#define WAIT_SIG1 PC5
#define WAIT_RESET_SIG1 PC6
#define CHANGE_BANK_ADDR_SIG1 PC7

#define USER_LED_SWITCH_SIG2 PD2
#define RD_SIG2 PD3
#define WR_SIG2 PD4
#define MREQ_SIG2 PD5
#define RESET_SIG2 PD6

#define MEM_OE_SIG2 RD_SIG2
#define MEM_WE_SIG2 WR_SIG2
#define MEM_CE_SIG2 MREQ_SIG2

#define IO_PORT_CONSOLE 0
#define IO_PORT_USER_LED_SWITCH 1
#define IO_PORT_MEMORY_BANK 2

#define IO_USER_DISABLE_LED 0
#define IO_USER_ENABLE_LED 1
#define IO_USER_SELECT_LED_MODE 2
#define IO_USER_SELECT_SWITCH_MODE 3

#define INLINE static __attribute__((always_inline)) inline

static const uint8_t bootloader_code[] = {
    0xAF, 0x4F, 0xED, 0x50, 0x21, 0x00, 0x01, 0x47,
    0xED, 0xB2, 0x15, 0x20, 0xFB, 0xC3, 0x00, 0x01,
};

// The currently selected memory bank
uint8_t memory_bank;

INLINE void acquire_data_bus() {    
    DDR_DATA_BUS = 0b11111111;
}

INLINE void release_data_bus() {
    PORT_DATA_BUS = 0b11111111;
    DDR_DATA_BUS = 0b00000000;
}

INLINE void acquire_address_bus() {
    DDR_ADDRESS_BUS |= 0b00001111;
}

INLINE void release_address_bus() {
    PORT_ADDRESS_BUS |= 0b00001111;
    DDR_ADDRESS_BUS &= 0b11110000;
}

static void setup_pins() {
    // Data bus D0 - D7
    // Enable pull-ups
    DDR_DATA_BUS = 0b00000000;
    PORT_DATA_BUS = 0b11111111;

    // Address bus A0 - A3
    // Enable pull-ups
    DDR_ADDRESS_BUS &= 0b11110000;
    PORT_ADDRESS_BUS |= 0b00001111;

    /*
     *  SIG1 group
     *
     *  PC0 - I2C SCL (input, pull-up)
     *  PC1 - I2C SDA (input, pull-up)
     *  PC2 - #INT line (output, high)
     *  PC3 - #BUSRQ line (output, high)
     *  PC4 - #BUSACK line (input, no pull-up)
     *  PC5 - #WAIT line (input, no pull-up)
     *  PC6 - #WAIT_RESET line (output, high)
     *  PC7 - CHANGE_BANK_ADDR line (output, low)
     */
    DDR_SIG1 = 0b11001100;
    PORT_SIG1 = 0b01001111;

    /*
     *  SIG2 group
     *
     *  PD0 - USART RxD (input, pull-up)
     *  PD1 - USART TxD (output, high)
     *  PD2 - User switch (input, pull-up)
     *  PD3 - #RD (input, pull-up)
     *  PD4 - #WR (input, pull-up)
     *  PD5 - #MREQ (input, pull-up)
     *  PD6 - #RESET (output, low)
     *  PD7 - #CLK (output, high)
     */
    DDR_SIG2 = 0b11000010;
    PORT_SIG2 = 0b10111111;

    // Interrupt request on falling edge of INT2
    EICRA = (1 << ISC21)
          | (0 << ISC20);

    // Enable pin change interrupt 2, which enables PCINT16-23
    // #WAIT line is connected to PC5 which corresponds to PCINT21
    PCICR |= (1 << PCIE2);

    // Enable pin change interrupt on PCINT21 (PC5 aka #WAIT line)
    PCMSK2 |= (1 << PCINT21);
}

 INLINE void signal_io_complete() {
    // Pulse #WAIT_RESET line
    PIN_SIG1 |= (1 << WAIT_RESET_SIG1);
    PIN_SIG1 |= (1 << WAIT_RESET_SIG1);
}

INLINE void wait_z80_read_complete() {
    while (!(PIN_SIG2 & (1 << RD_SIG2))) {}
}

static void console_io_handler(uint8_t input_request) {
    if (input_request) {
        // TODO

    } else {
        // Send one character via USART
        UDR0 = PIN_DATA_BUS;
    }
}

static void user_io_handler(uint8_t input_request) {
    if (input_request) {
        acquire_data_bus();
        // Put led / switch status on data bus
        PORT_DATA_BUS = (PIN_SIG2 & (1 << USER_LED_SWITCH_SIG2)) == 0;

    } else {
        // Read IO data from data bus
        const uint8_t cmd = PIN_DATA_BUS;
        switch (cmd) {
            case IO_USER_DISABLE_LED:
                // Check if LED mode is selected
                if (DDR_SIG2 & (1 << USER_LED_SWITCH_SIG2)) {
                    PORT_SIG2 |= (1 << USER_LED_SWITCH_SIG2);
                }
                break;

            case IO_USER_ENABLE_LED:
                // Check if LED mode is selected
                if (DDR_SIG2 & (1 << USER_LED_SWITCH_SIG2)) {
                    PORT_SIG2 &= ~(1 << USER_LED_SWITCH_SIG2);    
                }
                break;

            case IO_USER_SELECT_LED_MODE:
                // Select output mode on USER_LED_SWITCH line
                DDR_SIG2 |= (1 << USER_LED_SWITCH_SIG2);
                // Disable LED
                PORT_SIG2 |= (1 << USER_LED_SWITCH_SIG2);
                break;

            case IO_USER_SELECT_SWITCH_MODE:
                // Select input mode on USER_LED_SWITCH line
                DDR_SIG2 &= ~(1 << USER_LED_SWITCH_SIG2);
                // Activate pull-up
                PORT_SIG2 |= (1 << USER_LED_SWITCH_SIG2);
                break;
        }
    }
}

static void memory_bank_io_handler(uint8_t input_request) {
    if (input_request) {
        acquire_data_bus();
        PORT_DATA_BUS = memory_bank;
        
    } else {
        // New bank index is on data bus
        memory_bank = PIN_DATA_BUS;

        // Pulse CHANGE_BANK_ADDR
        PIN_SIG1 |= (1 << CHANGE_BANK_ADDR_SIG1);
        PIN_SIG1 |= (1 << CHANGE_BANK_ADDR_SIG1);
    }
}

// IO request handler
// Will be called on the falling edge of #WAIT
ISR(PCINT2_vect) {
    // Read the IO port address from address bus
    const uint8_t io_port = PIN_ADDRESS_BUS & 0b00001111;
    // Determine direction of request
    const uint8_t input_request = (PIN_SIG2 & (1 << RD_SIG2)) == 0;

    switch (io_port) {
        case IO_PORT_CONSOLE:
            console_io_handler(input_request);
            break;

        case IO_PORT_USER_LED_SWITCH:
            user_io_handler(input_request);
            break;

        case IO_PORT_MEMORY_BANK:
            memory_bank_io_handler(input_request);
            break;
    }

    // Signal bus request
    PORT_SIG1 &= ~(1 << BUSRQ_SIG1);
    signal_io_complete();
    // Wait for Z80 to release the bus
    while (PIN_SIG1 & (1 << BUSACK_SIG1)) {}

    if (input_request) {
        release_data_bus();
    }

    // Signal bus release
    PORT_SIG1 |= (1 << BUSRQ_SIG1);
}

static void setup_usart() {
    // Setup baud rate
    //
    // See atmega datasheet page 178
    const uint16_t ubrr = (F_CPU / (16U * USART_BAUD_RATE)) - 1;
    UBRR0H = (ubrr >> 8) & 0x0F;
    UBRR0L = ubrr & 0xFF;

    UCSR0B = (1 << RXEN0)    // enable receiver
           | (1 << TXEN0)    // enable transmitter
           | (0 << UCSZ02);  // 8-bit char size
    UCSR0C = (0 << UMSEL01)  // async. mode
           | (0 << UMSEL00)
           | (0 << UPM01)    // disable parity check
           | (0 << UPM00)
           | (0 << USBS0)    // 1 stop bit
           | (1 << UCSZ01)   // 8-bit char size cont.
           | (1 << UCSZ00)
           | (0 << UCPOL0);  // clock polarity
}

INLINE void bus_request() {
    PORT_SIG1 &= ~(1 << BUSRQ_SIG1);
    while (PIN_SIG1 & (1 << BUSACK_SIG1)) {}
}

INLINE void bus_release() {
    PORT_SIG1 |= (1 << BUSRQ_SIG1);
}

static void set_memory_bank(uint8_t bank) {
    // Save memory bank index
    memory_bank = bank;

    acquire_data_bus();
    
    // Put bank address on data bus
    PORT_DATA_BUS = bank;

    // Pulse CHANGE_BANK_ADDR
    // One clock cycle (~50ns) should be long enough.
    //
    // See SN74HC373 datasheet page 4
    PIN_SIG1 |= (1 << CHANGE_BANK_ADDR_SIG1);
    PIN_SIG1 |= (1 << CHANGE_BANK_ADDR_SIG1);

    release_data_bus();
}

static void switch_memory_bank(uint8_t bank) {
    bus_request();
    set_memory_bank(bank);
    bus_release();
}

static void upload_bootloader() {
    const uint8_t bootloader_size = sizeof(bootloader_code);

    // See AS6C4008 datasheet page 3

    // Acquire #OE, #WE and #CE lines of memory IC
    DDR_SIG2 |= (1 << MEM_OE_SIG2)
             |  (1 << MEM_WE_SIG2)
             |  (1 << MEM_CE_SIG2);

    PORT_SIG2 |= (1 << MEM_OE_SIG2)  // Set #OE high to ensure that the memory IC won't drive the data bus
              |  (1 << MEM_WE_SIG2)  // Set #WE high to ensure that no data will be written to memory by accedent
              |  (1 << MEM_CE_SIG2); // Set #CE to high to disable memory IC

    acquire_data_bus();
    acquire_address_bus();

    // Enable memory IC
    PORT_SIG2 &= ~(1 << MEM_CE_SIG2);

    for (uint8_t i = 0; i < bootloader_size; i++) {
        // Put next byte on the data bus
        PORT_DATA_BUS = bootloader_code[i];
        // Put address on the address bus
        PORT_ADDRESS_BUS = (i & 0b00001111);

        // Pulse #WE
        // The width of a write pulse must be at least 45ns long.
        // With a clock speed of 20Mhz one cycle is 50ns long so no
        // additional delay is required before setting the #WE line
        // high again.
        //
        // See AS6C4008 datasheet pages 4 and 6
        PIN_SIG2 |= (1 << MEM_WE_SIG2);
        PIN_SIG2 |= (1 << MEM_WE_SIG2);
    }

    release_address_bus();
    release_data_bus();

    // Set #OE, #WE, #CE lines to high (will enable pull-up on release)
    PORT_SIG2 |= (1 << MEM_OE_SIG2)
              |  (1 << MEM_WE_SIG2)
              |  (1 << MEM_CE_SIG2);

    // Release #OE, #WE, #CE lines
    DDR_SIG2 &= ~(
          (1 << MEM_OE_SIG2)
        | (1 << MEM_WE_SIG2)
        | (1 << MEM_CE_SIG2)
    );
}

INLINE void enable_z80_cpu() {
    PORT_SIG2 |= (1 << RESET_SIG2);
}

INLINE void disable_z80_cpu() {
    PORT_SIG2 &= ~(1 << RESET_SIG2);
}

static void setup_z80_clock() {
    // Set output compare to zero. This will give us
    // a frequency of f_clk / [2N * (1 + OCR2A)] = f_clk / 2, where
    // N = prescaling factor (=1)
    //
    // See atmega datasheet page 152
    OCR2A = 0;

    TCCR2A = (0 << COM2A1)   // Toggle OC2A (PD7) on compare match
           | (1 << COM2A0)
           | (0 << COM2B1)   // Disconnect OC2B
           | (0 << COM2B0)
           | (1 << WGM21)    // CTC mode (bits 1-0)
           | (0 << WGM20);

    TCCR2B = (0 << FOC2A)    // Disable force output compare A
           | (0 << FOC2B)    // Disable force output compare B
           | (0 << WGM22)    // CTC mode (bit 2)
           | (0 << CS22)     // No clock source (timer is stopped)
           | (0 << CS21)
           | (0 << CS20);
}

INLINE void start_z80_clock() {
    TCCR2B |= (0 << CS22)     // Use system clock without prescaler (/1)
           |  (0 << CS21)
           |  (1 << CS20);
}

INLINE void stop_z80_clock() {
    TCCR2B &= ~((1 << CS22)     // No clock source (timer is stopped)
           |    (1 << CS21)
           |    (1 << CS20));
}

INLINE uint8_t usart_recv_8() {
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0;
}

INLINE uint16_t usart_recv_16() {
    const uint8_t h = usart_recv_8();
    return (h << 8) + usart_recv_8();
}

// Reads 256 bytes if bytes == 0
static void usart_recv_block(uint8_t *dst, uint8_t bytes) {
    do {
        while (!(UCSR0A & (1 << RXC0))) {}
        *dst++ = UDR0;
    } while (--bytes);
}

INLINE void usart_send_8(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data;
}

// Uploads 256 bytes if block_size == 0
static void upload_block(const uint8_t *block, uint8_t block_size, uint8_t lock_bus) {
    do {
        // Signal bus release from previous loop
        PORT_SIG1 |= (1 << BUSRQ_SIG1);

        // Wait for an IO request from Z80
        while (PIN_SIG1 & (1 << WAIT_SIG1)) {}

        acquire_data_bus();
        PORT_DATA_BUS = *block++;

        // Signal bus request
        PORT_SIG1 &= ~(1 << BUSRQ_SIG1);
        signal_io_complete();
        // Wait for Z80 to release the bus
        while (PIN_SIG1 & (1 << BUSACK_SIG1)) {}

        release_data_bus();
    } while (--block_size);

    if (!lock_bus) {
        // Signal bus release
        PORT_SIG1 |= (1 << BUSRQ_SIG1);
    }
}

static uint8_t upload_z80_binary_from_usart() {
    uint8_t read_buffer[256];
    
    uint16_t binary_size = usart_recv_16();
    uint8_t block_count = binary_size >> 8;
    if ((binary_size & 0xff) > 0) {
        block_count++;
    }

    // Upload block count
    upload_block(&block_count, 1, 0);
    // Ready for receiving blocks
    usart_send_8(1);

    while (binary_size > 0) {
        // chunk_size is one less than actual size
        // to allow 256 byte sized chunks
        const uint8_t chunk_size = usart_recv_8();
        if (chunk_size > binary_size - 1) {
            // Reject current block (invalid size)
            usart_send_8(0);
            return 0;
        }

        usart_recv_block(read_buffer, chunk_size + 1);
        const uint8_t checksum_recv = usart_recv_8();

        uint8_t checksum_calc = 0u;
        uint8_t i = chunk_size;
        do {
            checksum_calc ^= read_buffer[i];
        } while (i--);

        if (checksum_recv == checksum_calc) {
            // Keep bus locked after last byte uploaded
            upload_block(read_buffer, 0, binary_size - 1 == chunk_size);
            // Acknowledge current block
            usart_send_8(1);

        } else {
            // Reject current block (checksum failed)
            usart_send_8(0);
            return 0;
        }

        binary_size -= chunk_size;
        binary_size--;
    }

    return 1;
}

static void load_from_usart_command() {
    stop_z80_clock();
    disable_z80_cpu();
    cli();
    upload_bootloader();
    enable_z80_cpu();
    start_z80_clock();
    if (upload_z80_binary_from_usart()) {
        sei();
        bus_release();

    } else {
        stop_z80_clock();
        disable_z80_cpu();
    }
}

int main() {
    cli();
    
    setup_pins();
    setup_usart();
    setup_z80_clock();
    set_memory_bank(2);

    for(;;) {
        const uint8_t command = usart_recv_8();
        switch (command) {
            case 1:
                load_from_usart_command();
                break;
        }
    }

    return 0;
}
