/*
  pins_arduino.h - Pin definition functions for Arduino
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  $Id: wiring.h 249 2007-02-03 16:52:51Z mellis $
*/

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

#define NUM_DIGITAL_PINS            35
#define NUM_ANALOG_INPUTS           0
#define analogInputToDigitalPin(p)  (-1)

#define digitalPinHasPWM(p)         ((p) == 5 || (p) == 6 || (p) == 8 || (p) == 9 || (p) == 10 || (p) == 30)

const static uint8_t SS   = 10;
const static uint8_t MOSI = 11;
const static uint8_t MISO = 12;
const static uint8_t SCK  = 13;

const static uint8_t LED_BUILTIN = 13;

#define digitalPinToPCICR(p)    ( (((p) >= 14) && ((p) <= 17)) || \
                                  (((p) >= 20) && ((p) <= 27)) || \
                                  (((p) >= 31) && ((p) <= 34)) ? (&PCICR) : ((uint8_t *)0) )
								  
#define digitalPinToPCICRbit(p) ( (((p) >= 20) && ((p) <= 27)) ? 0 : 1 )

#define digitalPinToPCMSK(p)    ( (((p) >= 20) && ((p) <= 27)) ? (&PCMSK0) : \
								  (((p) >= 14) && ((p) <= 17)) || \
                                  (((p) >= 31) && ((p) <= 34)) ? (&PCMSK1) : ((uint8_t *)0) )

#define digitalPinToPCMSKbit(p) ( (((p) >= 20) && ((p) <= 27)) ? ((p) - 20) : \
								  (((p) >= 14) && ((p) <= 17)) ? ((p) - 14) : ((p) - 27) )

#ifdef ARDUINO_MAIN

// On the Arduino board, digital pins are also used
// for the analog output (software PWM).  Analog input
// pins are a separate set.

// ATMEL ATMEGA162 / ARDUINO
//
//                  +-\/-+
//  PWM(0)  (D 8) PB0  1|    |40  VCC
//  PWM(2)  (D 9) PB1  2|    |39  PA0 (D14)
//          (D19) PB2  3|    |38  PA1 (D15)
//          (D18) PB3  4|    |37  PA2 (D16)
// PWM(3B)  (D10) PB4  5|    |36  PA3 (D17)
//          (D11) PB5  6|    |35  PA4 (D31)
//          (D12) PB6  7|    |34  PA5 (D32)
//          (D13) PB7  8|    |33  PA6 (D33)
//                RST  9|    |32  PA7 (D34)
//          (D 0) PD0 10|    |31  PE0 (D28)
//          (D 1) PD1 11|    |30  PE1 (D29)
//          (D 2) PD2 12|    |29  PE2 (D30) PWM(1B)
//          (D 3) PD3 13|    |28  PC7 (D27)
// PWM(3A)  (D 5) PD4 14|    |27  PC6 (D26)
// PWM(1A)  (D 6) PD5 15|    |26  PC5 (D25)
//          (D 4) PD6 16|    |25  PC4 (D24)
//          (D 7) PD7 17|    |24  PC3 (D23)
//                XT2 18|    |23  PC2 (D22)
//                XT1 19|    |22  PC1 (D21)
//                GND 20|    |21  PC0 (D20)
//                      +----+
//


// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
const uint16_t PROGMEM port_to_mode_PGM[] = {
	NOT_A_PORT,
	(uint16_t) &DDRA,
	(uint16_t) &DDRB,
	(uint16_t) &DDRC,
	(uint16_t) &DDRD,
	(uint16_t) &DDRE,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
	NOT_A_PORT,
	(uint16_t) &PORTA,
	(uint16_t) &PORTB,
	(uint16_t) &PORTC,
	(uint16_t) &PORTD,
	(uint16_t) &PORTE,
};

const uint16_t PROGMEM port_to_input_PGM[] = {
	NOT_A_PORT,
	(uint16_t) &PINA,
	(uint16_t) &PINB,
	(uint16_t) &PINC,
	(uint16_t) &PIND,
	(uint16_t) &PINE,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
	PD, /* 0 */
	PD,
	PD,
	PD,
	PD,
	PD,
	PD,
	PD,
	PB, /* 8 */
	PB,
	PB,
	PB,
	PB,
	PB,
	PA, /* 14 */
	PA,
	PA,
	PA,
	PB, /* 18 */
	PB,
	PC, /* 20 */
	PC,
	PC,
	PC,
	PC,
	PC,
	PC,
	PC,
	PE, /* 28 */
	PE,
	PE,
	PA, /* 31 */
	PA,
	PA,
	PA,
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
	_BV(0), /* 0, port D */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(6),
	_BV(4),
	_BV(5),
	_BV(7),
	_BV(0), /* 8, port B */
	_BV(1),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 14, port A */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(3), /* 18, port B */
	_BV(2),
	_BV(0), /* 20, port C */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 28, port E */
	_BV(1),
	_BV(2),
	_BV(4), /* 31, port A */
	_BV(5),
	_BV(6),
	_BV(7),
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
	NOT_ON_TIMER, /* 0 - port D */
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	TIMER3A,
	TIMER1A,
	NOT_ON_TIMER,
	TIMER0A, /* 8 - port B */
	TIMER2A,
	TIMER3B,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,/* 14, port A */
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER, /* 18, port B */
	NOT_ON_TIMER,
	NOT_ON_TIMER, /* 20, port C */
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER, /* 28, port E */
	NOT_ON_TIMER,
	TIMER1B,
	NOT_ON_TIMER, /* 31, port A */
	NOT_ON_TIMER,
	NOT_ON_TIMER,
	NOT_ON_TIMER,
};

#endif

#endif