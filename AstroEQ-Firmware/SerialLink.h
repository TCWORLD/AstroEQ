#ifndef __SERIAL_LINK_H__
#define __SERIAL_LINK_H__

#include "AstroEQ.h"

void Serial_initialise(const unsigned long baud);
byte Serial_available(void);
char Serial_read(void);
void Serial_write(char ch);
void Serial_writeStr(char* str);
void Serial_writeArr(char* arr, byte len);

#endif //__SERIAL_LINK_H__

