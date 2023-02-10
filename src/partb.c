#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
//* part B: Now with interrupts */
void Initialize() {
    cli(); // disable global interrupts
    DDRB |= (1 << DDB5); // Set onboard LED pin 13 as output
    PORTB |= (1 << PORTB0); // Enable pull-up resistor on PB1
    DDRB &= ~(1 << DDB0); // Set button pin PB0 as input

    TCCR1B &= ~(1<<ICES1); // detect falling edge (button press)
    TIMSK1 |= (1<<ICIE1); // Enable input capture interrupt

//    Timer1 Setup
    TCCR1B |= (1<<CS10);
    TCCR1B &= ~(1<<CS11);
    TCCR1B &= ~(1<<CS12);

    TIFR1 |= (1<<ICF1); // Clear input capture flag
    sei(); // Enable global interrupts
}

ISR(TIMER1_CAPT_vect) {

    // set led based on if button pressed
    if (PINB & (1 << PINB0)) {
        PORTB |= (1 << PORTB5);
        TCCR1B &= ~(1<<ICES1); // set to detect falling edge
    } else {
        PORTB &= ~(1 << PORTB5);
        TCCR1B |= (1<<ICES1); // set to detect rising edge
    }

}

int main() {
    Initialize();
    while (1);
}