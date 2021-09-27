#define F_CPU 20000000            // AVR clock frequency in Hz, used by util/delay.h
#define UART_BAUD_RATE 9600       // baud = F_CPU / (16 * (UBRR0 + 1)
#define UART_BAUD_REGISTER 9600   // UART_BAUD_RATE = F_CPU / (16 * (UART_BAUD_REGISTER + 1)

#include <avr/io.h>
#include <util/delay.h>
  
int main() {
    DDRD |= (1<<2);        // set USER LED pin PD2 to output
    while (1) {
        PORTD |= (1<<2);   // drive USER LED pin PD2 high
        _delay_ms(500);    // delay 100 ms
        PORTD &= ~(1<<2);  // drive USER LED pin PD2 low
        _delay_ms(500);    // delay 900 ms
    }
}


/* Initializing the USART 0 port, on pins 14 and 15. */
void USART_Init( unsigned int baud_rate ) {
    unsigned long baud_register; 

    // Set baud rate:
    // baud_rate = F_CPU / (16 * (baud_register + 1)
    // baud_rate * (16 * baud_register + 16) = F_CPU
    // baud_rate * 16 * baud_register + 16 * baud_rate =  F_CPU
    // baud_register0 = (F_CPU - 16 * baud_rate) / (baud_rate * 16)
    baud_register = 

    UBRR0H = (unsigned char)(baud>>8); 
    UBRR0L = (unsigned char)baud;

    /* Enable receiver and transmitter */ 
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);

    /* Set frame format: 8data, 2stop bit */ 
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}


/* Sending one byte of data on the USART 0 port, on pins 14 and 15. */
void USART_Transmit( unsigned char data ) {
    /* Wait for empty transmit buffer */ 
    while ( !( UCSR0A & (1<<UDRE0)) ) ;
    
    /* Put data into buffer, sends the data */
    UDR0 = data;
}

/* Recieving one byte of data on the USART 0 port, on pins 14 and 15. 
   The function is blocking until the data is recieved. */
unsigned int USART_Receive( void ) {
    unsigned char status, resh, resl; 
    
    /* Wait for data to be received */ 
    while ( !(UCSR0A & (1<<RXC0)) ) ;
    
    /* Get status and 9th bit, then data */
    /* from buffer */ 
    status = UCSR0A; 
    resh = UCSR0B; 
    resl = UDR0;

    /* If error, return -1 */
    if ( status & (1<<FE0)|(1<<DOR0)|(1<<UPE0) )
        return -1;

    /* Filter the 9th bit, then return */ 
    resh = (resh >> 1) & 0x01;
    return ((resh << 8) | resl);
}

/* Cleaning up the receive buffer used for the USART 0 port, on pins 14 and 15. */
void USART_Clean_receive_buffer( void ) {
    unsigned char dummy;
    while ( UCSR0A & (1<<RXC0) ) dummy = UDR0;
}

