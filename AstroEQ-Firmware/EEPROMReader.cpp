
#include <avr/eeprom.h>
#include "EEPROMReader.h"
 
byte EEPROM_readByte(byte address)
{
  return eeprom_read_byte((byte*) address);
}

unsigned int EEPROM_readInt(byte address)
{
  TwoBytes fetcher;
  fetcher.array[0] = EEPROM_readByte(address);
  fetcher.array[1] = EEPROM_readByte(address+1);
  return fetcher.integer;
}
unsigned long EEPROM_readLong(byte address)
{
  FourBytes fetcher;
  fetcher.array[0] = EEPROM_readInt(address);
  fetcher.array[1] = EEPROM_readInt(address+2);
  return fetcher.integer;
}

void EEPROM_readString(char* string, byte len, byte address)
{
  for(byte i = 0; i < len; i++) {
    string[i] = EEPROM_readByte(address++);
  }
}

void EEPROM_writeByte(byte val, byte address)
{
  return eeprom_write_byte((byte*) address, val);
}

void EEPROM_writeInt(unsigned int val, byte address)
{
  TwoBytes storer = {val};
  EEPROM_writeByte(storer.array[0], address);
  EEPROM_writeByte(storer.array[1], address+1);
}
void EEPROM_writeLong(unsigned long val, byte address)
{
  FourBytes storer = {val};
  EEPROM_writeInt(storer.array[0], address);
  EEPROM_writeInt(storer.array[1], address+2);
}
void EEPROM_writeString(const char* string, byte len, byte address)
{
  for(byte i = 0; i < len; i++) {
    EEPROM_writeByte(string[i], address+i);
  }
}

