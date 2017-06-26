
#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "AstroEQ.h"

byte EEPROM_readByte(unsigned int address);
unsigned int EEPROM_readInt(unsigned int address);
unsigned long EEPROM_readLong(unsigned int address);
void EEPROM_readString(char* string, byte len, unsigned int address);
void EEPROM_readAccelTable(AccelTableStruct* table, byte elements, unsigned int address);
void EEPROM_writeByte(byte val,unsigned int address);
void EEPROM_writeInt(unsigned int val,unsigned int address);
void EEPROM_writeLong(unsigned long val,unsigned int address);
void EEPROM_writeString(const char* string, byte len, unsigned int address);
void EEPROM_writeAccelTable(AccelTableStruct* table, byte elements, unsigned int address);

#endif //__EEPROM_H__
