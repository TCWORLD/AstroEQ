

#ifndef __PINMAPPINGS_H__
#define __PINMAPPINGS_H__

//
// Pin Mappings
//

//For custom pin map, add a copy of the #if ... #el section below,
//give it a name, e.g. titled "PIN_MAP_EXAMPLECUSTOMMAP" and add a
//commented out #define below. You can then create a pull request on
//Github if you want to share the new map.
//To use a mapping, simply uncomment the required #define. If none
//are uncommented, it will default to the standard AstroEQ mapping
//for the current AVR.

//#define PIN_MAP_EXAMPLECUSTOMMAP   //Example, copy this one to form own

#if defined(PIN_MAP_EXAMPLECUSTOMMAP)
//
//---- User Configurable Pin Definitions -----
//

//GPIO Pins:
//VCC (Header Pin 5)
#define gpioPin_0_Define 21  //IO0 (Header Pin 4) - GPIO Pin
#define gpioPin_1_Define 22  //IO1 (Header Pin 3) - GPIO Pin
#define gpioPin_2_Define 23  //IO2 (Header Pin 2) - GPIO Pin
//GND (Header Pin 1)

//PWM Pins:
#define pwmPin_Define 10

//Status Pins:
#define statusPin_Define 13

//Motor Driver Pins:
#define resetPin_0_Define 55 //Analog 1
#define resetPin_1_Define 54 //Analog 0

#define dirPin_0_Define 3
#define dirPin_1_Define 7

#define enablePin_0_Define 4
#define enablePin_1_Define 8

#define stepPin_0_Define 5
#define stepPin_1_Define 12

#define modePins0_0_Define 15
#define modePins1_0_Define 16
#define modePins2_0_Define 17
#define modePins0_1_Define 20
#define modePins1_1_Define 19
#define modePins2_1_Define 18

//ST4 Pins:
#define ST4AddPin_0_Define 50
#define ST4AddPin_1_Define 51
#define ST4SubPin_0_Define 53
#define ST4SubPin_1_Define 52


//
//----- End User Configurable Pin Assignments -----
//


#elif defined(PIN_MAP_MKS_GEN_L)
//
//---- MKS Gen L and RAMPS 1.4 Pin Definitions -----
//

//GPIO Pins:
//VCC (Header Pin 5)
#define gpioPin_0_Define 49  //IO0 - GPIO Pin
#define gpioPin_1_Define 33  //IO1 - GPIO Pin
#define gpioPin_2_Define 41  //IO2 - GPIO Pin
//GND (Header Pin 1)

//PWM Pins:
#define pwmPin_Define 9

//Status Pins:
#define statusPin_Define 13
#define statusPinShadow_Define 11 //Duplicate of status LED for boards which don't have on-board LED

//Motor Driver Pins:
#define resetPin_0_Define 35 //Unused on RAMPS
#define resetPin_1_Define 37 //Unused on RAMPS

#define dirPin_0_Define 55
#define dirPin_1_Define 61

#define enablePin_0_Define 38
#define enablePin_1_Define 56

#define stepPin_0_Define 54
#define stepPin_1_Define 60

#define modePins0_0_Define 25
#define modePins1_0_Define 27
#define modePins2_0_Define 29
#define modePins0_1_Define 16
#define modePins1_1_Define 17
#define modePins2_1_Define 23

//ST4 Pins:
#define ST4AddPin_0_Define 50
#define ST4AddPin_1_Define 51
#define ST4SubPin_0_Define 53
#define ST4SubPin_1_Define 52


//
//----- End MKS Gen L Pin Assignments -----
//


#elif defined(__AVR_ATmega162__)

//
//----- Pin Definitions for ATMega162 Variants -----
//Warning: D20 to D27 inclusive are NOT allowed

//GPIO Header:
                             //VCC (Header Pin 5)
#define gpioPin_0_Define 2   //IO0 (Header Pin 4) [ATMega PD2] - GPIO Pin
#define gpioPin_1_Define 29  //IO1 (Header Pin 3) [ATMega PE1] - GPIO Pin
#define gpioPin_2_Define 28  //IO2 (Header Pin 2) [ATMega PE0] - GPIO Pin
                             //GND (Header Pin 1)

//PWM Pins:
#define pwmPin_Define 9

//Status Pins:
#define statusPin_Define 13 

//Motor Driver Pins:
#define resetPin_0_Define 15
#define resetPin_1_Define 14

#define dirPin_0_Define 3
#define dirPin_1_Define 7

#define enablePin_0_Define 4
#define enablePin_1_Define 8

#define stepPin_0_Define 5
#define stepPin_1_Define 30

#define modePins0_0_Define 6
#define modePins1_0_Define 17
#define modePins2_0_Define 16
#define modePins0_1_Define 10
#define modePins1_1_Define 18
#define modePins2_1_Define 19

//ST4 Pins:
#define ST4AddPin_0_Define 34
#define ST4AddPin_1_Define 33
#define ST4SubPin_0_Define 31
#define ST4SubPin_1_Define 32


//
//----- End Pin Definitions for ATMega162 Variants -----
//


#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

//---- Pin Definitions for ATMegaXXX0 Variants -----

//GPIO Pins:
                             //VCC (Header Pin 5)
#define gpioPin_0_Define 21  //IO0 (Header Pin 4) [ATMega PD0] - GPIO Pin
#define gpioPin_1_Define 22  //IO1 (Header Pin 3) [ATMega PA0] - GPIO Pin
#define gpioPin_2_Define 23  //IO2 (Header Pin 2) [ATMega PA1] - GPIO Pin
                             //GND (Header Pin 1)

//PWM Pins:
#define pwmPin_Define 10

//Status Pins:
#define statusPin_Define 13

//Motor Driver Pins:
#define resetPin_0_Define 55 //Analog 1
#define resetPin_1_Define 54 //Analog 0

#define dirPin_0_Define 3
#define dirPin_1_Define 7

#define enablePin_0_Define 4
#define enablePin_1_Define 8

#define stepPin_0_Define 5
#define stepPin_1_Define 12

#define modePins0_0_Define 15
#define modePins1_0_Define 16
#define modePins2_0_Define 17
#define modePins0_1_Define 20
#define modePins1_1_Define 19
#define modePins2_1_Define 18

//ST4 Pins:
#define ST4AddPin_0_Define 50
#define ST4AddPin_1_Define 51
#define ST4SubPin_0_Define 53
#define ST4SubPin_1_Define 52


//
//----- End Pin Definitions for ATMegaXXX0 Variants -----
//


#endif






//
//
//----- Do not modify anything below this line! ---------------------------------------
//
//


//
// Useful Macros
//

#define setPinDir(p,d) {if(d){*digitalPinToDirectionReg((p)) |= _BV(digitalPinToBit((p)));}else{*digitalPinToDirectionReg((p)) &= ~_BV(digitalPinToBit((p)));}}
#define setPinValue(p,v) {if(v){*digitalPinToPortReg((p)) |= _BV(digitalPinToBit((p)));}else{*digitalPinToPortReg((p)) &= ~_BV(digitalPinToBit((p)));}}
#define getPinValue(p) (!!(*digitalPinToPinReg((p)) & _BV(digitalPinToBit((p)))))
#define togglePin(p) {*digitalPinToPortReg((p)) ^= _BV(digitalPinToBit((p)));}

//
//Advanced Hand Controller SPI Pins (Must be ST4 pins!):
//

#define SPIClockPin_Define ST4SubPin_1_Define
#define SPIMISOPin_Define  ST4AddPin_0_Define
#define SPIMOSIPin_Define  ST4AddPin_1_Define
#define SPISSnPin_Define   ST4SubPin_0_Define


//
//Register Unification
//

#if (CS10 != CS30) || (CS11 != CS31) || (CS12 != CS32)
#error incorrect assumption about prescale bits being equal between timer 1 and 3.
#endif
#define CSn0 CS10
#define CSn1 CS11
#define CSn2 CS12



#if defined(__AVR_ATmega162__)

#ifndef TCCR2A
#define TCCR2A TCCR2
#endif

#ifndef TCCR2B
#define TCCR2B TCCR2
#endif

#ifndef COM2A0
#define COM2A0 COM20
#endif

#ifndef COM2A1
#define COM2A1 COM21
#endif

#ifndef OCR2A
#define OCR2A OCR2
#endif

#ifndef USART0_TX_vect
#define USART0_TX_vect USART0_TXC_vect
#endif
#ifndef USART0_RX_vect
#define USART0_RX_vect USART0_RXC_vect
#endif

#ifndef USART1_TX_vect
#define USART1_TX_vect USART1_TXC_vect
#endif
#ifndef USART1_RX_vect
#define USART1_RX_vect USART1_RXC_vect
#endif

//Pick some otherwise unused registers for GPIOR. Must be < 0x3F
#define GPIOR0 PORTC
#define GPIOR1 OCR0
#define GPIOR2 TCNT0

#define PCICR GICR

#ifndef TIMSK3
#define TIMSK3 ETIMSK
#endif
#ifndef TIMSK1
#define TIMSK1 TIMSK
#endif
#ifndef ICIE3
#define ICIE3 TICIE3
#endif
#ifndef ICIE1
#define ICIE1 TICIE1
#endif

#endif

//
// Pin to Register Lookup Tables
//

#if defined(__AVR_ATmega162__)

#define digitalPinToPortReg(P) \
((((P) >= 14 && (P) <= 17) || ((P) >= 31 && (P) <= 34)) ? &PORTA : \
((((P) >= 8  && (P) <= 13) || ((P) >= 18 && (P) <= 19)) ? &PORTB : \
((((P) >= 20 && (P) <= 27)                            ) ? &PORTC : \
((((P) <= 7              )                            ) ? &PORTD : &PORTE ))))

#define digitalPinToDirectionReg(P) \
((((P) >= 14 && (P) <= 17) || ((P) >= 31 && (P) <= 34)) ? &DDRA : \
((((P) >= 8  && (P) <= 13) || ((P) >= 18 && (P) <= 19)) ? &DDRB : \
((((P) >= 20 && (P) <= 27)                            ) ? &DDRC : \
((((P) <= 7              )                            ) ? &DDRD : &DDRE ))))

#define digitalPinToPinReg(P) \
((((P) >= 14 && (P) <= 17) || ((P) >= 31 && (P) <= 34)) ? &PINA : \
((((P) >= 8  && (P) <= 13) || ((P) >= 18 && (P) <= 19)) ? &PINB : \
((((P) >= 20 && (P) <= 27)                            ) ? &PINC : \
((((P) <= 7              )                            ) ? &PIND : &PINE ))))

#define digitalPinToBit(P) \
(((P) >=  0 && (P) <=  3) ? (P)      : \
(((P) >= 10 && (P) <= 13) ? (P) - 6  : \
(((P) >=  8 && (P) <=  9) ? (P) - 8  : \
(((P) >= 18 && (P) <= 19) ? 21 - (P) : \
(((P) >= 20 && (P) <= 27) ? (P) - 20 : \
(((P) >= 28 && (P) <= 30) ? (P) - 28 : \
(((P) >= 31 && (P) <= 34) ? (P) - 27 : \
(((P) >= 14 && (P) <= 17) ? (P) - 14 : \
(((P) >=  5 && (P) <=  6) ? (P) - 1  : \
(((P) == 7              ) ? (P)      : 6 ))))))))))


#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)


#define digitalPinToPortReg(P) \
((((P) >= 22 && (P) <= 29)                            ) ? &PORTA : \
((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &PORTB : \
((((P) >= 30 && (P) <= 37)                            ) ? &PORTC : \
((((P) >= 18 && (P) <= 21) || ((P) == 38             )) ? &PORTD : \
((((P) >= 0  && (P) <= 3 ) || ((P) == 5              )) ? &PORTE : \
((((P) >= 54 && (P) <= 61)                            ) ? &PORTF : \
((((P) >= 39 && (P) <= 41) || ((P) == 4              )) ? &PORTG : \
((((P) >= 6  && (P) <= 9 ) || ((P) >= 16 && (P) <= 17)) ? &PORTH : \
((((P) >= 14 && (P) <= 15)                            ) ? &PORTJ : \
((((P) >= 62 && (P) <= 69)                            ) ? &PORTK : &PORTL))))))))))

#define digitalPinToDirectionReg(P) \
((((P) >= 22 && (P) <= 29)                            ) ? &DDRA : \
((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &DDRB : \
((((P) >= 30 && (P) <= 37)                            ) ? &DDRC : \
((((P) >= 18 && (P) <= 21) || ((P) == 38             )) ? &DDRD : \
((((P) >= 0  && (P) <= 3 ) || ((P) == 5              )) ? &DDRE : \
((((P) >= 54 && (P) <= 61)                            ) ? &DDRF : \
((((P) >= 39 && (P) <= 41) || ((P) == 4              )) ? &DDRG : \
((((P) >= 6  && (P) <= 9 ) || ((P) >= 16 && (P) <= 17)) ? &DDRH : \
((((P) >= 14 && (P) <= 15)                            ) ? &DDRJ : \
((((P) >= 62 && (P) <= 69)                            ) ? &DDRK : &DDRL))))))))))

#define digitalPinToPinReg(P) \
((((P) >= 22 && (P) <= 29)                            ) ? &PINA : \
((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &PINB : \
((((P) >= 30 && (P) <= 37)                            ) ? &PINC : \
((((P) >= 18 && (P) <= 21) || ((P) == 38             )) ? &PIND : \
((((P) >= 0  && (P) <= 3 ) || ((P) == 5              )) ? &PINE : \
((((P) >= 54 && (P) <= 61)                            ) ? &PINF : \
((((P) >= 39 && (P) <= 41) || ((P) == 4              )) ? &PING : \
((((P) >= 6  && (P) <= 9 ) || ((P) >= 16 && (P) <= 17)) ? &PINH : \
((((P) >= 14 && (P) <= 15)                            ) ? &PINJ : \
((((P) >= 62 && (P) <= 69)                            ) ? &PINK : &PINL))))))))))

#define digitalPinToBit(P) \
(((P) >=  7 && (P) <=  9) ? (P) - 3 : \
(((P) >= 10 && (P) <= 13) ? (P) - 6 : \
(((P) >= 22 && (P) <= 29) ? (P) - 22 : \
(((P) >= 30 && (P) <= 37) ? 37 - (P) : \
(((P) >= 39 && (P) <= 41) ? 41 - (P) : \
(((P) >= 42 && (P) <= 49) ? 49 - (P) : \
(((P) >= 50 && (P) <= 53) ? 53 - (P) : \
(((P) >= 54 && (P) <= 61) ? (P) - 54 : \
(((P) >= 62 && (P) <= 69) ? (P) - 62 : \
(((P) >= 0  && (P) <= 1 ) ? (P)      : \
(((P) >= 14 && (P) <= 15) ? 15 - (P) : \
(((P) >= 16 && (P) <= 17) ? 17 - (P) : \
(((P) >= 20 && (P) <= 21) ? 21 - (P) : \
(((P) == 19             ) ?       2  : \
(((P) >= 5  && (P) <= 6 ) ?       3  : \
(((P) == 18             ) ?       3  : \
(((P) == 2              ) ?       4  : \
(((P) >= 3  && (P) <= 4 ) ?       5  : 7))))))))))))))))))



#endif


#endif //__PINMAPPINGS_H__

