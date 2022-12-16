#include "common.h"
#include "lib_usart.h"

uint8_t usart_receive_buffer_overflow = 0;
uint8_t usart_receive_frame_error = 0;


void setup_usart() {
    // Setup baud rate
    //
    // See atmega datasheet page 178
    const uint16_t ubrr = (F_CPU / (16UL * USART_BAUD_RATE)) - 1;
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

void setup_usart_timeout_timer() {
    // Use 16-bit timer/counter1
    TCCR1A = (0 << COM1A1)    // Disconnect OC1A (PD5)
           | (0 << COM1A0)
           | (0 << COM1B1)    // Disconnect OC1B (PD4)
           | (0 << COM1B0)
           | (0 << WGM11)     // CTC mode (bits 1-0)
           | (0 << WGM10);

    TCCR1B = (0 << ICNC1)     // Disable input capture noise canceller
           | (0 << ICES1)     // Input capture select falling edge (ignored)
           | (0 << WGM13)     // CTC mode (bits 2-3)
           | (1 << WGM12)
           | (0 << CS12)      // No clock source (timer is stopped)
           | (0 << CS11)
           | (0 << CS10);
}

void start_usart_timeout_timer(uint16_t bytes_expected) {
    // F_CPU: the main clock frequency 
    // 10: 8bit transfer on USART is about 10 signal (start bit, stop bit)
    // 5: maybe the other end (written in python) is lagging behind, not sending the response immediately
    //    (or the ATMega is doing some extra cycles between receiving the data and stopping the timer) 
    // USART_BAUD_RATE: baud rate in bit per sec
    // 1024: we set an 1024 prescaler in the timer (see below)
    const uint8_t cycles_per_byte = F_CPU * 10 * 5 / USART_BAUD_RATE / 1024;
    const uint16_t max_bytes = 65535 / cycles_per_byte;

    // Zero counter
    TCNT1 = 0;
 
    // Set timeout counter
    if (bytes_expected <= max_bytes) {
        // Set counter according to bytes_expected
        OCR1A = (uint16_t) (bytes_expected * cycles_per_byte);
    } else {
        // Set maximum counter value possible
        OCR1A = (uint16_t) 65535;
   }

    // Clear timeout flag
    TIFR1 |= (1 << OCF1A);

    // Start timer
    TCCR1B |= (1 << CS12)        // Use system clock with /1024 prescaler
           |  (0 << CS11)
           |  (1 << CS10);
}

void stop_usart_timeout_timer() {
    TCCR1B &= ~((1 << CS12)      // No clock source (timer is stopped)
           |    (1 << CS11)
           |    (1 << CS10));
}

INLINE uint8_t usart_rx_buffer_ready() {
    return (UCSR0A & (1 << RXC0)) > 0;
}

INLINE uint8_t usart_tx_buffer_ready() {
    return (UCSR0A & (1 << UDRE0)) > 0;
}

INLINE uint8_t usart_timed_out() {
    return (TIFR1 & (1 << OCF1A)) > 0;
}

uint8_t usart_recv_8() {
    while (!usart_rx_buffer_ready() && !usart_timed_out()) {}
    const uint8_t status = UCSR0A;
    if(status & (1<<FE0)) {
        usart_receive_frame_error = 1;
    }
    if(status & (1<<DOR0)) {
        usart_receive_buffer_overflow = 1;
    }
    return UDR0;
}

uint16_t usart_recv_16() {
    const uint8_t h = usart_recv_8();
    return (h << 8) + usart_recv_8();
}

// Reads 256 bytes if bytes == 0
void usart_recv_block(uint8_t *dst, uint8_t bytes) {
    do {
        *dst++ = usart_recv_8();
    } while ( --bytes && !usart_timed_out() );
}

void usart_send_8(uint8_t data) {
    while (!usart_tx_buffer_ready()) {}
    UDR0 = data;
}


void flush_receive_buffer() {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    uint8_t dummy; 
    while ( UCSR0A & (1<<RXC0) ) dummy = UDR0;
    #pragma GCC diagnostic pop
}

// we have a 2 bytes long receive buffer, you should always make sure 
// to check the error flags if you read more than 2 bytes
uint8_t validate_usart_communication() {
    stop_usart_timeout_timer();
    if(!usart_receive_frame_error && !usart_receive_buffer_overflow && !usart_timed_out()) {
        return ERR_OK;
    }
    uint8_t err = usart_timed_out() ? ERR_TIMEOUT : ERR_OK;
    if(usart_receive_frame_error || usart_receive_buffer_overflow) {
        err = usart_receive_frame_error ? ERR_USART_FRAME_ERROR : ERR_USART_BUFFER_OVERFLOW;
    }
    flush_receive_buffer();
    usart_send_8(err);
    usart_receive_frame_error = 0;
    usart_receive_buffer_overflow = 0;
    TIFR1 |= (1 << OCF1A); // Clear timeout flag
    return err;
}


/**
 * Fetching a single block of data through usart, applying checksum.
 * Returns ERR_OK if everything is right. 
 */
uint8_t fetch_block_from_usart(uint8_t *read_buffer, uint8_t* chunk_size, uint8_t max_size) {

    // chunk_size is one less than actual size
    // to allow 256 byte sized chunks

    start_usart_timeout_timer(1); 
    *chunk_size = usart_recv_8();
    uint8_t err_usart = validate_usart_communication();
    if(err_usart != ERR_OK) return err_usart;


    if(max_size < *chunk_size) {
        flush_receive_buffer();
        usart_send_8(ERR_INVALID_SIZE);
        return ERR_INVALID_SIZE;
    }


    // 1 checksum byte + chunk_size (one less than actual size) 
    start_usart_timeout_timer((uint16_t) (*chunk_size + 2));
    // chunk_size + 1 may overflow uint8_t but it's handled in usart_recv_block
    usart_recv_block(read_buffer, *chunk_size + 1);
    const uint8_t checksum_recv = usart_recv_8();
    err_usart = validate_usart_communication();
    if(err_usart != ERR_OK) return err_usart;

    uint8_t checksum_calc = 0u;
    uint8_t i = *chunk_size;
    do {
        checksum_calc ^= read_buffer[i];
    } while (i--);

    if (checksum_recv != checksum_calc) {
        // Reject current block (checksum failed)
        flush_receive_buffer();
        usart_send_8(ERR_CHECKSUM_FAILED);
        return ERR_CHECKSUM_FAILED;
    }
    return ERR_OK;
}

void usart_send_block(uint8_t* bytes, uint8_t length) {
    uint8_t checksum_calc = 0u;
    usart_send_8(length);
    // length is one smaller than the actual length, enabling us to send 1..256 bytes
    for(uint8_t i=0; i<=length; i++) {
        checksum_calc ^= bytes[i];
        usart_send_8(bytes[i]);
    }
    usart_send_8(checksum_calc);
}
