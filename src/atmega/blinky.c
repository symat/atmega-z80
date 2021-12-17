#include <avr/io.h>
#include <util/delay.h>
  
int main() {
    DDRD |= (1<<2);        // set USER LED pin PD2 to output
    while (1) {
        PORTD &= ~(1<<2);  // drive USER LED pin PD2 low (light on)
        _delay_ms(500);    // delay 500 ms
        PORTD |= (1<<2);   // drive USER LED pin PD2 high (light off)
        _delay_ms(500);    // delay 500 ms
    }
}