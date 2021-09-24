#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define INLINE __attribute__((always_inline)) inline

#define USART_BAUD_RATE 9600

#define BUSRQ PC3
#define BUSACK PC4
#define WAIT PC5
#define WAIT_RESET PC6
#define CHANGE_BANK_ADDR PC7

#define USER_LED_SWITCH PD2
#define Z80_RD PD3
#define Z80_WR PD4
#define Z80_MREQ PD5
#define Z80_RESET PD6

static const uint8_t bootloader_code[] = {
    0xAF, 0x4F, 0xED, 0x50, 0x21, 0x00, 0x01, 0x47,
    0xED, 0xB2, 0x15, 0x20, 0xFB, 0xC3, 0x00, 0x01,
};

static void setup_pins() {
    // PA0 - PA7 - Data bus D0 - D7
    // Enable pull-ups
    DDRA = 0b00000000;
    PORTA = 0b11111111;

    // PB0 - PB3 - Address bus A0 - A3
    // Enable pull-ups
    DDRB = 0b00000000;
    PORTB |= 0b00001111;

    /*
     *  PC0 - I2C SCL (input, pull-up)
     *  PC1 - I2C SDA (input, pull-up)
     *  PC2 - Z80 #INT line (output, high)
     *  PC3 - Z80 #BUSRQ line (output, high)
     *  PC4 - Z80 #BUSACK line (input, no pull-up)
     *  PC5 - Z80 #WAIT line (output, high)
     *  PC6 - #WAIT_RESET line (output, high)
     *  PC7 - CHANGE_BANK_ADDR line (output, low)
     */
    DDRC = 0b11001100;
    PORTC = 0b01101111;

    /*
    *  PD0 - USART RxD (input, pull-up)
    *  PD1 - USART TxD (output, high)
    *  PD2 - User switch (input, pull-up)
    *  PD3 - Z80 #RD (input, pull-up)
    *  PD4 - Z80 #WR (input, pull-up)
    *  PD5 - Z80 #MREQ (input, pull-up)
    *  PD6 - Z80 #RESET (output, low)
    *  PD7 - Z80 #CLK (output, high)
    */
    DDRD = 0b11000010;
    PORTD = 0b10111111;

    // Interrupt request on falling edge of INT2
    EICRA = (1 << ISC21)
          | (0 << ISC20);

    // Enable pin change interrupt 2, which enables PCINT16-23
    // #WAIT line is connected to PC5 which corresponds to PCINT21
    PCICR |= (1 << PCIE2);

    // Enable pin change interrupt on PCINT21 (PC5 aka #WAIT line)
    PCMSK2 |= (1 << PCINT21);
}

 INLINE static void signal_io_complete() {
    // Pulse WAIT_RESET line
    PINC |= (1 << WAIT_RESET);
    PINC |= (1 << WAIT_RESET);
}

INLINE static void wait_z80_read_complete() {
    while (!(PIND & (1 << Z80_RD))) {}
}

static void console_io_handler() {
    // TODO
}

static void user_io_handler(uint8_t input_request) {
    if (input_request) {
        // Acquire data bus
        DDRA = 0b11111111;
        // Put switch status on the bus
        PORTA = (PORTD & (1 << USER_LED_SWITCH)) == 0;

    } else {
        // Read IO data from data bus
        const uint8_t cmd = PINA;
        switch (cmd) {
            case 0: // Disable LED
                // Check if LED mode is selected
                if (DDRD & (1 << USER_LED_SWITCH)) {
                    PORTD |= (1 << USER_LED_SWITCH);
                }
                break;

            case 1: // Enable LED
                // Check if LED mode is selected
                if (DDRD & (1 << USER_LED_SWITCH)) {
                    PORTD &= ~(1 << USER_LED_SWITCH);    
                }

            case 2: // Select LED mode
                // Select output mode on USER_LED_SWITCH line
                DDRD |= (1 << USER_LED_SWITCH);
                // Disable LED
                PORTD |= (1 << USER_LED_SWITCH);
                break;

            case 3: // Select switch mode
                // Select input mode on USER_LED_SWITCH line
                DDRD &= ~(1 << USER_LED_SWITCH);
                // Activate pull-up
                PORTD |= (1 << USER_LED_SWITCH);
                break;
        }
    }
}

// IO request handler.
// Will be called on the falling edge of #WAIT.
ISR(PCINT2_vect) {
    // Read the IO port address from address bus
    const uint8_t io_port = PINB & 0b00001111;
    // Determine direction of request
    const uint8_t input_request = (PIND & (1 << Z80_RD)) == 0;

    switch (io_port) {
        case 0: // Console
            console_io_handler(input_request);
            break;

        case 1: // User switch / LED
            user_io_handler(input_request);
            break;
    }

    // Signal bus request
    PORTC &= ~(1 << BUSRQ);
    signal_io_complete();
    // Wait for Z80 to release the bus
    while (PINC & (1 << BUSACK)) {}

    if (input_request) {
        // It's an input request so
        // release data bus and re-enable pull-ups
        PORTA = 0b11111111;
        DDRA = 0b00000000;
    }

    // Release bus
    PORTC |= (1 << BUSRQ);
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

static void bus_request() {
    PORTC &= ~(1 << BUSRQ);
    while (PINC & (1 << BUSACK)) {}
}

INLINE static void bus_release() {
    PORTC |= (1 << BUSRQ);
}

static void set_memory_bank(uint8_t bank) {
    // Acquire data bus
    DDRA = 0b11111111;
    // Put bank address on data bus
    PORTA = bank;

    // Pulse CHANGE_BANK_ADDR
    // One clock cycle (~50ns) should be long enough.
    //
    // See SN74HC373 datasheet page 4
    PINC |= (1 << CHANGE_BANK_ADDR);
    PINC |= (1 << CHANGE_BANK_ADDR);

    // Set data bus to high (will enable pull-ups on release)
    PORTA = 0b11111111;
    // Release data bus
    DDRA = 0b00000000;
}

static void switch_memory_bank(uint8_t bank) {
    bus_request();
    set_memory_bank(bank);
    bus_release();
}

static void upload_bootloader() {
    const uint8_t bootloader_size = sizeof(bootloader_code);

    // Acquire data bus
    DDRA = 0b11111111;
    // Acquire address bus
    DDRB |= 0b1111;

    // Acquire of #WR and #MREQ (#CE on memory chip) lines
    DDRD |= (1 << Z80_WR) | (1 << Z80_MREQ);

    // Set #MREQ to low to enable memory chip.
    // This is safe because #OE is pulled high by the internal pull-up
    // the entire time so the memory chip won't drive the data bus
    // while #WR is high.
    //
    // See AS6C4008 datasheet page 3
    PORTD &= ~(1 << Z80_MREQ);

    // Set #WR to high
    PORTD |= (1 << Z80_WR);
    for (uint8_t i = 0; i < bootloader_size; i++) {
        // Put next byte on the data bus
        PORTA = bootloader_code[i];
        // Put address on the address bus
        PORTB = (i & 0b00001111);

        // Pulse #WR
        // The width of a write pulse must be at least 45ns long.
        // With a clock speed of 20Mhz one cycle is 50ns long so no
        // additional delay is required before setting the #WR line
        // to high state again.
        //
        // See AS6C4008 datasheet pages 4 and 6
        PIND |= (1 << Z80_WR);
        PIND |= (1 << Z80_WR);
    }

    // Set #WR and #MREQ to high (will enable pull-ups on release)
    PORTD |= (1 << Z80_WR) | (1 << Z80_MREQ);
    // Release #WR and #MREQ
    DDRD &= ~((1 << Z80_WR) | (1 << Z80_MREQ));

    // Set address bus to high (will enable pull-ups on release)
    PORTB |= 0b00001111;
    // Release address bus
    DDRB &= 0b11110000;
    
    // Set data bus to high (will enable pull-ups on release)
    PORTA = 0b11111111;
    // Release data bus
    DDRA = 0b00000000;
}

INLINE static void enable_z80_cpu() {
    PORTD &= ~(1 << Z80_RESET);
}

INLINE static void disable_z80_cpu() {
    PORTD |= (1 << Z80_RESET);
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

static void start_z80_clock() {
    TCCR2B |= (0 << CS22)     // Use system clock without prescaler (/1)
           |  (0 << CS21)
           |  (1 << CS20);
}

INLINE static void stop_z80_clock() {
    TCCR2B &= ~((1 << CS22)     // No clock source (timer is stopped)
           |    (1 << CS21)
           |    (1 << CS20));
}

INLINE static uint8_t usart_recv_8() {
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0;
}

INLINE static uint16_t usart_recv_16() {
    const uint8_t h = usart_recv_8();
    return (h << 8) + usart_recv_8();
}

static void usart_recv_block(uint8_t *dst, uint8_t bytes) {
    while (bytes) {
        while (!(UCSR0A & (1 << RXC0))) {}
        *dst++ = UDR0;
    }
}

INLINE static void usart_send_8(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data;
}

static void upload_block(const uint8_t *block, uint8_t block_size) {
    while (--block_size) {
        // Wait for an IO request from Z80
        while (PINC & (1 << WAIT)) {}

        // Put data on the data bus
        DDRA = 0b11111111;
        PORTA = *block++;

        signal_io_complete();

        // Wait for Z80 to finish reading
        while(!(PIND & (1 << Z80_RD))) {}

        // Release data bus
        DDRA = 0b00000000;
        PORTA = 0b11111111;
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
    upload_block(&block_count, 1);
    // Ready for receiving the blocks
    usart_send_8(1);

    while (binary_size > 0) {
        const uint8_t chunk_size = usart_recv_8();
        usart_recv_block(read_buffer, chunk_size);
        const uint8_t checksum_recv = usart_recv_8();

        uint8_t checksum_calc = 0u;
        for (int i = 0; i < chunk_size; i++) {
            checksum_calc ^= read_buffer[i];
        }

        if (checksum_recv == checksum_calc) {
            upload_block(read_buffer, 0);
            if (binary_size == chunk_size) {
                // Immediately request bus access after uploading the last chunk
                // to prevent the Z80 to execute code
                bus_request();
            }
            usart_send_8(1);

        } else {
            usart_send_8(0);
            return 0;
        }

        binary_size -= chunk_size;
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
