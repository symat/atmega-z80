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