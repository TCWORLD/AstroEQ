
#ifndef __SERIAL_LINK_H__
#define __SERIAL_LINK_H__

#include "AstroEQ.h"

//Serial Defines
//Select the required serial port with the #define below.
//For ATMega162, valid options are 0. USART1 on the ATMega162 variants must not be used for AstroEQ.
//For Arduino Mega, valid options are 0,1,(2,3) the latter two are untested but should work.
#define SERIALn 0

//Serial Functions
void Serial_initialise(const unsigned long baud);
void Serial_disable();

//SPI Functions
void SPI_initialise();
void SPI_disable();

//Common Functions
byte Serial_available(void);
void Serial_clear(void);
char Serial_read(void);
void Serial_flush(void);
void Serial_write(char ch);
void Serial_writeStr(char* str);
void Serial_writeArr(char* arr, byte len);

#endif //__SERIAL_LINK_H__

