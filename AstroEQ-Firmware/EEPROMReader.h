#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "AstroEQ.h"

byte EEPROM_readByte(byte);
unsigned int EEPROM_readInt(byte);
unsigned long EEPROM_readLong(byte);
void EEPROM_readString(char* string, byte len, byte address);
void EEPROM_writeByte(byte,byte);
void EEPROM_writeInt(unsigned int,byte);
void EEPROM_writeLong(unsigned long,byte);
void EEPROM_writeString(const char* string, byte len, byte address);

#endif //__EEPROM_H__
