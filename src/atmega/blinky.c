#include <avr/io.h>
#include <util/delay.h>

#define LED PD2

int main() {
    DDRD |= (1<<LED);        // set USER LED pin PD2 to output
    
	TCCR1B = (1<<CS10) | (1<<CS12) | (1 << WGM12); //set the pre-scalar as 1024, CTC mode
	
    OCR1A = F_CPU / 1024UL / 2UL; 	   //500ms delay
	TCNT1 = 0;    
    
    
    while (1) {
		//If flag is set toggle the led	
		while((TIFR1 & (1<<OCF1A)) == 0);// wait till the timer overflow flag is SET
		PORTD ^= (1<< LED);
		TCNT1 = 0; 
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag 
    }
}