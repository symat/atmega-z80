#include "common.h"
#include "lib_usart.h"


static const uint8_t bootloader_code[] = {
    0xAF, 0x4F, 0xED, 0x50, 0x21, 0x00, 0x01, 0x47,
    0xED, 0xB2, 0x15, 0x20, 0xFB, 0xC3, 0x00, 0x01,
};

// The currently selected memory bank
uint8_t memory_bank;


static void setup_pins() {
    // Pull #RESET line down to prevent Z80
    // to drive any lines during setup
    DDR_SIG2 |= (1 << RESET_SIG2);
    PORT_SIG2 &= ~(1 << RESET_SIG2);

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
}

static void blink(int blinks) {
    DDRD |= (1<<2);        // set USER LED pin PD2 to output
    for (int i=0; i<blinks; i++) {
        PORTD &= ~(1<<2);  // drive USER LED pin PD2 low (light on)
        _delay_ms(500);    // delay 500 ms
        PORTD |= (1<<2);   // drive USER LED pin PD2 high (light off)
        _delay_ms(500);    // delay 500 ms
    }
}


// Uploads 256 bytes if block_size == 0
static void upload_block(const uint8_t *block, uint8_t block_size, uint8_t lock_bus) {
    /*
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
    */
}


static uint8_t upload_z80_binary_from_usart() {
    uint8_t read_buffer[256];

    start_usart_timeout_timer(2);
    uint16_t binary_size = usart_recv_16();
    uint8_t err_usart = validate_usart_communication();
    if(err_usart != ERR_OK) return err_usart;

    uint8_t block_count = binary_size >> 8;
    if ((binary_size & 0xff) > 0) {
        block_count++;
    }
 
    // Upload block count
    upload_block(&block_count, 1, 0);
    // Ready for receiving blocks
    usart_send_8(ERR_OK);
    usart_send_8(block_count);

    while (binary_size > 0) {
        uint8_t chunk_size;
        err_usart = fetch_block_from_usart(read_buffer, &chunk_size, 255);
        if(err_usart != ERR_OK) return err_usart;

        if ( (block_count > 1 && chunk_size == 255) || (block_count == 1 && chunk_size == binary_size - 1) ) {
            // chunk_size is OK
            // Keep bus locked after last byte uploaded
            upload_block(read_buffer, 0, block_count == 1);
            usart_send_8(ERR_OK);
        } else {
            // Reject current block (invalid size)
            usart_send_8(ERR_INVALID_SIZE);
            flush_receive_buffer();
            return ERR_INVALID_SIZE;
        }

        binary_size -= chunk_size; 
        binary_size--; // chunk_size is one less than actual size
        block_count--;
    }

    return ERR_OK;
}


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


INLINE void bus_request() {
    PORT_SIG1 &= ~(1 << BUSRQ_SIG1);
    while (PIN_SIG1 & (1 << BUSACK_SIG1)) {}
}

INLINE void bus_release() {
    PORT_SIG1 |= (1 << BUSRQ_SIG1);
}

static void upload_to_ram(uint8_t* code_to_upload, uint8_t length) {
    // return;
    bus_request();

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

    for (uint8_t i = 0; i < length; i++) {
        // Put next byte on the data bus
        PORT_DATA_BUS = code_to_upload[i];
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

    bus_release();
}

static void download_16byte_from_ram(uint8_t *read_buffer) {
 
    bus_request(); // asking the Z80 to give us the buses
    release_data_bus();  // we only read the data bus

    // See AS6C4008 datasheet page 3

    // Acquire #OE, #WE and #CE lines of memory IC
    DDR_SIG2 |= (1 << MEM_OE_SIG2)
             |  (1 << MEM_WE_SIG2)
             |  (1 << MEM_CE_SIG2);

    PORT_SIG2 |= (1 << MEM_OE_SIG2)  // Set #OE high to ensure that the memory IC won't drive the data bus
              |  (1 << MEM_WE_SIG2)  // Set #WE high to ensure that no data will be written to memory by accedent
              |  (1 << MEM_CE_SIG2); // Set #CE to high to disable memory IC

    acquire_address_bus();

    // Enable memory IC, and enable output. write remains disabled
    PORT_SIG2 &= ~((1 << MEM_CE_SIG2) | (1 << MEM_OE_SIG2));

    for (uint8_t i = 0; i < 16; i++) {
        // Put address on the address bus
        PORT_ADDRESS_BUS = (i & 0b00001111);

        // wait one pulse, maing sure the output appeared
        __asm("nop");

        // read the next byte on the data bus
        read_buffer[i] = PORT_DATA_BUS;
    }

    release_address_bus();
 
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

    bus_release();
}


static void load_z80_binary_from_usart_command() {
/*
    stop_z80_clock();
    disable_z80_cpu();
    cli();
    upload_bootloader();
    enable_z80_cpu();
    start_z80_clock();
    if (upload_z80_binary_from_usart() == ERR_OK) {
        sei();
        bus_release();
    } else {
        stop_z80_clock();
        disable_z80_cpu();
    }
    */
}

int main() {
    cli();
    
    setup_pins();
    setup_usart();
    setup_usart_timeout_timer();
    blink(1);

    for(;;) {
        const uint8_t command = usart_recv_8();
        switch (command) {
            case CMD_ECHO:
                usart_send_8(usart_recv_8());
                break;
            case CMD_LOAD_Z80_BINARY_FROM_USART:
                upload_z80_binary_from_usart();
                //load_z80_binary_from_usart_command();
                break;
            case CMD_UPLOAD_CODE_TO_RAM:
                uint8_t ram_upload_buffer[16];
                uint8_t length;
                uint8_t err_usart = fetch_block_from_usart(ram_upload_buffer, &length, 15);
                if(err_usart == ERR_OK) {
                    upload_to_ram(ram_upload_buffer, length);
                    usart_send_8(ERR_OK);
                }
                break;
            case CMD_DOWNLOAD_FIRST_16_BYTE_FROM_RAM:
                uint8_t ram_download_buffer[16];
                download_16byte_from_ram(ram_download_buffer);
                usart_send_block(ram_download_buffer, 15); // length 15 means 16 bytes
                break;

            default:
                flush_receive_buffer();
                usart_send_8(ERR_UNKNOWN_COMMAND);
        }
    }

}
