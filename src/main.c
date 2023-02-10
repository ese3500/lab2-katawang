#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)

/*--------------------Libraries---------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "uart.h"

/*--------------------Variables---------------------------*/
char String[25];
// variables here for interrupt
int overflows = 0;
int start_tick;
int end_tick;
int duration;
int not_pressed;
int not_press_start;
int space_detected = 0;
int not_press_overflows = 0;
int letter[5] = {0, 0, 0, 0, 0};
int ct = 0;

/*-----------------------------------------------------------*/
/* part D: Dash or Dot */
void Initialize() {
    cli(); // disable global interrupts
    DDRB |= (1 << DDB5); // Set onboard LED pin 13 as output
    DDRB |= (1 << DDB1); // Set LED pin 9 as output
    DDRB |= (1 << DDB2); // Set LED pin 10 as output
    PORTB |= (1 << PORTB0); // Enable pull-up resistor on PB1
    DDRB &= ~(1 << DDB0); // Set button pin PB0 as input

    TCCR1B &= ~(1 << ICES1); // detect falling edge (button press)
    TIMSK1 |= (1 << ICIE1); // Enable input capture interrupt
    TIMSK1 |= (1 << TOIE1);// enable overflow interrupt

//    Timer1 Setup-- clock/256 from prescaler
    TCCR1B &= ~(1 << CS10);
    TCCR1B &= ~(1 << CS11);
    TCCR1B |= (1 << CS12);

    TIFR1 |= (1 << ICF1); // Clear input capture flag
    sei(); // Enable global interrupts
}

// overflow interrupt
ISR(TIMER1_OVF_vect) {
    if (PINB & (1 << PINB0)) {
        // if the TCNT1 register overflows, increment overflow
        overflows += 1;
        not_press_overflows = 0;
    } else {
        overflows = 0;
        not_press_overflows += 1;
    }

}
// button press interrupt
ISR(TIMER1_CAPT_vect) {
    overflows = 0;
    not_press_overflows = 0;

    // set led based on if button pressed
    if (PINB & (1 << PINB0)) {
        _delay_ms(30);
        start_tick = TCNT1;
        PORTB |= (1 << PORTB5);
        TCCR1B &= ~(1 << ICES1); // set to detect falling edge
    } else {
        end_tick = TCNT1;
        PORTB &= ~(1 << PORTB5);
        TCCR1B |= (1 << ICES1); // set to detect rising edge
    }

}

int main(void) {
// do all printing in main
    UART_init(BAUD_PRESCALER);
    Initialize();

    while (1) {
        not_press_start = TCNT1;

        if (overflows == 0) {
            duration = end_tick - start_tick;
        } else {
            duration = (65535 - start_tick) + (overflows - 1) * 65535 + end_tick;
        }
        if (not_press_overflows == 0) {
            not_pressed = start_tick - end_tick;
        } else {
            not_pressed = (65535 - not_press_start) + (not_press_overflows - 1) * 65535 + end_tick;
        }

        // dot-- 3840 ticks to 24576 ticks = 30ms to 200ms
        // 16MHz/256 = 62,500. 62500 * 0.030 = 1875 ticks in 30 ms
        if (duration >= 1875 && duration <= 12500) {

//            sprintf(String,"Dot %u, %u, %u, %u\n", start_tick, end_tick, duration, overflows);
//            UART_putstring(String);
            // set int arr
            letter[ct] = 1;
            ct += 1;
            // reset ticks?
            start_tick = TCNT1;
            end_tick = TCNT1;
            space_detected = 0;
            PORTB |= (1 << PORTB1);
            _delay_ms(50);
            PORTB &= ~(1 << PORTB1);

        } else if (duration > 12500 && duration <= 25000) {
            // dash-- 12500 ticks to 25000 ticks = 200ms to 400ms
//            sprintf(String, "Dash %u, %u, %u, %u\n", start_tick, end_tick, duration, overflows);
//            UART_putstring(String);
            letter[ct] = 2;
            ct += 1;
            start_tick = TCNT1;
            end_tick = TCNT1;
            space_detected = 0;
            PORTB |= (1 << PORTB2);
            _delay_ms(50);
            PORTB &= ~(1 << PORTB2);
        } else if (space_detected == 0 && not_pressed >= 30000) {
            // space-- greater than 30000 ticks

            // print out letter, first dot
            if (letter[0] == 1) {
                if (letter[1] == 1) {
                    if (letter[2] == 1) {
                        if (letter[3] == 1) {
                            if (letter[4] == 1) {
                                // * * * * * = 5
                                sprintf(String, "5\n");
                                UART_putstring(String);

                            } else if (letter[4] == 2) {
                                // * * * * - = 4
                                sprintf(String, "4\n");
                                UART_putstring(String);

                            } else {
                                // * * * * = S
                                sprintf(String, "H\n");
                                UART_putstring(String);
                            }

                        } else if (letter[3] == 2) {
                            if (letter[4] == 2) {
                                // * * * - - = 4
                                sprintf(String, "3\n");
                                UART_putstring(String);

                            } else {
                                // * * * - = V
                                sprintf(String, "V\n");
                                UART_putstring(String);
                            }

                        } else {
                            // * * * = S
                            sprintf(String, "S\n");
                            UART_putstring(String);
                        }
                    } else if (letter[2] == 2) {
                        if (letter[3] == 1) {
                            // * * - * = F
                            sprintf(String, "F\n");
                            UART_putstring(String);
                        } else if (letter[3] == 0) {
                            // * * - = U
                            sprintf(String, "U\n");
                            UART_putstring(String);
                        }

                    } else {
                        // * * = I
                        sprintf(String, "I\n");
                        UART_putstring(String);
                    }
                } else if (letter[1] == 2) {
                    if (letter[2] == 1) {
                        if (letter[3] == 1) {
                            // * - * * = L
                            sprintf(String, "L\n");
                            UART_putstring(String);
                        } else if (letter[3] == 0) {
                            // * - * = R
                            sprintf(String, "R\n");
                            UART_putstring(String);

                        }
                    } else if (letter[2] == 2) {
                        if (letter[3] == 1) {
                            // * - - * = P
                            sprintf(String, "P\n");
                            UART_putstring(String);

                        } else if (letter[3] == 2) {
                            // * - - - = J
                            sprintf(String, "J\n");
                            UART_putstring(String);

                        } else {
                            // * - - = W
                            sprintf(String, "W\n");
                            UART_putstring(String);

                        }
                    } else {
                        // * - = A
                        sprintf(String, "A\n");
                        UART_putstring(String);
                    }
                } else {
                    // * = E
                    sprintf(String, "E\n");
                    UART_putstring(String);
                }
            } else if (letter[0] == 2) {
                if (letter[1] == 1) {
                    if (letter[2] == 1) {
                        if (letter[3] == 1) {
                            if (letter[4] == 1) {
                                // - * * * = 6
                                sprintf(String, "6\n");
                                UART_putstring(String);
                            } else if (letter[4] == 0) {
                                // - * * * = B
                                sprintf(String, "B\n");
                                UART_putstring(String);
                            }
                        } else if (letter[3] == 2) {
                            if (letter[4] == 0) {
                                // - * * - = X
                                sprintf(String, "X\n");
                                UART_putstring(String);
                            }
                        } else {
                            // - * * = D
                            sprintf(String, "D\n");
                            UART_putstring(String);
                        }
                    } else if (letter[2] == 2) {
                        if (letter[3] == 1) {
                            // - * - * = C
                            sprintf(String, "C\n");
                            UART_putstring(String);
                        } else if (letter[3] == 2) {
                            // - * - - = Y
                            sprintf(String, "Y\n");
                            UART_putstring(String);
                        } else if (letter[4] == 0){
                            // - * - = K
                            sprintf(String, "K\n");
                            UART_putstring(String);
                        }

                    } else {
                        // - * = N
                        sprintf(String, "N\n");
                        UART_putstring(String);
                    }
                } else if (letter[1] == 2) {
                    if (letter[2] == 1) {
                        if (letter[3] == 1) {
                            if (letter[4] == 0) {
                                // - - * * = Z
                                sprintf(String, "Z\n");
                                UART_putstring(String);
                            } else if (letter[4] == 1) {
                                // - - * * * = 7
                                sprintf(String, "7\n");
                                UART_putstring(String);
                            }
                        } else if (letter[3] == 2) {
                            // - - * - = Q
                            sprintf(String, "Q\n");
                            UART_putstring(String);
                        } else {
                            // - - * = G
                            sprintf(String, "G\n");
                            UART_putstring(String);
                        }
                    } else if (letter[2] == 2) {
                        if (letter[3] == 1) {
                            if (letter[4] == 1) {
                                // - - - * * = 8
                                sprintf(String, "8\n");
                                UART_putstring(String);

                            }

                        } else if (letter[3] == 2) {
                            if (letter[4] == 1) {
                                // - - - - * = 9
                                sprintf(String, "9\n");
                                UART_putstring(String);

                            } else if (letter[4] == 2) {
                                // - - - - - = 0
                                sprintf(String, "0\n");
                                UART_putstring(String);

                            }

                        } else {
                            // - - - = O
                            sprintf(String, "O\n");
                            UART_putstring(String);

                        }
                    } else {
                        // - - = M
                        sprintf(String, "M\n");
                        UART_putstring(String);
                    }
                } else if (letter[1] == 0){
                    // - = T
                    sprintf(String, "T\n");
                    UART_putstring(String);
                }
            }
            letter[0] = 0;
            letter[1] = 0;
            letter[2] = 0;
            letter[3] = 0;
            letter[4] = 0;
            ct=0;
            start_tick = TCNT1;
            not_press_start = TCNT1;
            end_tick = TCNT1;
            space_detected = 1;
            _delay_ms(20);
        }

    }
}

/*--------------------Previous parts of lab commented out below---------------------------*/

/* part 1 */
//void Initialize() {
//    DDRB |= (1 << DDB1);
//    DDRB |= (1 << DDB2);
//    DDRB |= (1 << DDB3);
//    DDRB |= (1 << DDB4);
//}
//int main() {
//    Initialize();
//    while(1) {
//        PORTB |= (1 << PORTB1);
//        PORTB |= (1 << PORTB2);
//        PORTB |= (1 << PORTB3);
//        PORTB |= (1 << PORTB4);
//    }
//
//}

/* part 2 */
//int main() {
//    DDRD &= ~(1<<DDD7);
//    while(1) {
//        if (PIND & (1 << PIND7)) {
//            DDRB |= (1 << DDB1);
//            PORTB |= (1 << PORTB1);
//        } else {
//            DDRB |= (1 << DDB1);
//            PORTB &= ~(1 << PORTB1);
//
//        }
//    }
//}

/* part 3 */
//int main() {
//    DDRD &= ~(1 << DDD7);
//    DDRB |= (1 << DDB1);
//    PORTB |= (1 << PORTB1);
//
//    while(1) {
//        if (PIND & (1 << PIND7) && PINB & (1 << PINB1)) {
//            DDRB |= (1 << DDB1);
//            PORTB &= ~(1 << PORTB1);
//            DDRB |= (1 << DDB2);
//            PORTB |= (1 << PORTB2);
//            _delay_ms(1000);
//        } else if (PIND & (1 << PIND7) && PINB & (1 << PINB2)) {
//
//            DDRB |= (1 << DDB2);
//            PORTB &= ~(1 << PORTB2);
//            DDRB |= (1 << DDB3);
//            PORTB |= (1 << PORTB3);
//            _delay_ms(1000);
//
//        } else if (PIND & (1 << PIND7) && PINB & (1 << PINB3)) {
//            DDRB |= (1 << DDB3);
//            PORTB &= ~(1 << PORTB3);
//            DDRB |= (1 << DDB4);
//            PORTB |= (1 << PORTB4);
//            _delay_ms(1000);
//        } else if (PIND & (1 << PIND7) && PINB & (1 << PINB4)) {
//            DDRB |= (1 << DDB4);
//            PORTB &= ~(1 << PORTB4);
//            DDRB |= (1 << DDB1);
//            PORTB |= (1 << PORTB1);
//            _delay_ms(1000);
//        }
//    }
//}
//* part B: Now, with interrupts */
//#include <avr/interrupt.h>
//
//void Initialize() {
//    cli(); // disable global interrupts
//    DDRB |= (1 << DDB5); // Set onboard LED pin 13 as output
//    PORTB |= (1 << PORTB0); // Enable pull-up resistor on PB1
//    DDRB &= ~(1 << DDB0); // Set button pin PB0 as input
//
//    TCCR1B &= ~(1<<ICES1); // detect falling edge (button press)
//    TIMSK1 |= (1<<ICIE1); // Enable input capture interrupt
//
////    Timer1 Setup ??
//    TCCR1B |= (1<<CS10);
//    TCCR1B &= ~(1<<CS11);
//    TCCR1B &= ~(1<<CS12);
//
//    TIFR1 |= (1<<ICF1); // Clear input capture flag
//    sei(); // Enable global interrupts
//}
//
//ISR(TIMER1_CAPT_vect) {
//
//    // set led based on if button pressed
//    if (PINB & (1 << PINB0)) {
//        PORTB |= (1 << PORTB5);
//        TCCR1B &= ~(1<<ICES1); // set to detect falling edge
//    } else {
//        PORTB &= ~(1 << PORTB5);
//        TCCR1B |= (1<<ICES1); // set to detect rising edge
//    }
//
//}
//
//int main() {
//    Initialize();
//    while (1);
//}

////* part C: Dash or Dot */
//#include <avr/interrupt.h>
//#define F_CPU 16000000UL
//#define BAUD_RATE 9600
//#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)
//
///*--------------------Libraries---------------------------*/
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdbool.h>
//#include "uart.h"
//
///*--------------------Variables---------------------------*/
//char String[25];
//// variables here for interrupt
//int overflows = 0;
//int start_tick;
//int end_tick;
//int duration;
//int not_pressed;
//int not_press_start;
//int space_detected = 0;
//int not_press_overflows = 0;
//
///*-----------------------------------------------------------*/
//void Initialize() {
//    cli(); // disable global interrupts
//    DDRB |= (1 << DDB5); // Set onboard LED pin 13 as output
//    DDRB |= (1 << DDB1); // Set LED pin 9 as output
//    DDRB |= (1 << DDB2); // Set LED pin 10 as output
//    PORTB |= (1 << PORTB0); // Enable pull-up resistor on PB1
//    DDRB &= ~(1 << DDB0); // Set button pin PB0 as input
//
//    TCCR1B &= ~(1<<ICES1); // detect falling edge (button press)
//    TIMSK1 |= (1<<ICIE1); // Enable input capture interrupt
//    TIMSK1 |= (1<<TOIE1);// enable overflow interrupt
//
////    Timer1 Setup-- clock/256 from prescaler
//    TCCR1B &= ~(1<<CS10);
//    TCCR1B &= ~(1<<CS11);
//    TCCR1B |= (1<<CS12);
//
//    TIFR1 |= (1<<ICF1); // Clear input capture flag
//    sei(); // Enable global interrupts
//}
//
//// overflow interrupt
//ISR(TIMER1_OVF_vect) {
//    if (PINB & (1 << PINB0)) {
//        // if the TCNT1 register overflows, increment overflow
//        overflows += 1;
//        not_press_overflows = 0;
//    } else {
//        overflows = 0;
//        not_press_overflows += 1;
//    }
//
//}
//// button press interrupt
//ISR(TIMER1_CAPT_vect) {
//    overflows = 0;
//    not_press_overflows = 0;
//
//    // set led based on if button pressed
//    if (PINB & (1 << PINB0)) {
//        _delay_ms(30);
//        start_tick = TCNT1;
//        PORTB |= (1 << PORTB5);
//        TCCR1B &= ~(1<<ICES1); // set to detect falling edge
//    } else {
//        end_tick = TCNT1;
//        PORTB &= ~(1 << PORTB5);
//        TCCR1B |= (1<<ICES1); // set to detect rising edge
//    }
//
//}
//
//int main(void)
//{
//// do all printing in main
//    UART_init(BAUD_PRESCALER);
//    Initialize();
//
//
//
//    while(1)
//    {
//        not_press_start = TCNT1;
//
//        if (overflows == 0) {
//            duration = end_tick - start_tick;
//        } else {
//            duration = (65535 - start_tick) + (overflows - 1) * 65535 + end_tick;
//        }
//        if (not_press_overflows == 0) {
//            not_pressed = start_tick - end_tick;
//        } else {
//            not_pressed = (65535 - not_press_start) + (not_press_overflows - 1) * 65535 + end_tick;
//        }
//
//        // dot-- 3840 ticks to 24576 ticks = 30ms to 200ms
//        // 16MHz/256 = 62,500. 62500 * 0.030 = 1875 ticks in 30 ms
//        if (duration >= 1875 && duration <= 12500) {
//
//            sprintf(String,"Dot %u, %u, %u, %u\n", start_tick, end_tick, duration, overflows);
//            UART_putstring(String);
//            // reset ticks?
//            start_tick = TCNT1;
//            end_tick = TCNT1;
//            space_detected = 0;
//            PORTB |= (1 << PORTB1);
//            _delay_ms(50);
//            PORTB &= ~(1 << PORTB1);
//
//        } else if (duration > 12500 && duration <= 25000) {
//            // dash-- 12500 ticks to 25000 ticks = 200ms to 400ms
//            sprintf(String, "Dash %u, %u, %u, %u\n", start_tick, end_tick, duration, overflows);
//            UART_putstring(String);
//            start_tick = TCNT1;
//            end_tick = TCNT1;
//            space_detected = 0;
//            PORTB |= (1 << PORTB2);
//            _delay_ms(50);
//            PORTB &= ~(1 << PORTB2);
//        } else if (space_detected == 0 && not_pressed >= 30000) {
//            // space-- greater than 30000 ticks
//            sprintf(String,"Space %u, %u, %u, %u\n", not_pressed, start_tick, end_tick, not_press_overflows);
//            UART_putstring(String);
//            start_tick = TCNT1;
//            not_press_start = TCNT1;
//            end_tick = TCNT1;
//            space_detected = 1;
//            _delay_ms(20);
//        }
//
//    }
//}


