#ifndef __SERIAL_LINK_H__
#define __SERIAL_LINK_H__

#include "AstroEQ.h"

//Select the required serial port with the #define below.
//For ATMega162, valid options are 0,(1) the latter is untested and should work, but requires pin mappings to be changed
//For Arduino Mega, valid options are 0,1,(2,3) the latter two are untested but should work.
#define SERIALn 0

//Functions
void Serial_initialise(const unsigned long baud);
byte Serial_available(void);
char Serial_read(void);
void Serial_write(char ch);
void Serial_writeStr(char* str);
void Serial_writeArr(char* arr, byte len);

#endif //__SERIAL_LINK_H__

