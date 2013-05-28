
#include <avr/eeprom.h>
#include "Arduino.h"
#include "EEPROMReader.h"
 
uint8_t EEPROMReader::readByte(byte address)
{
  return eeprom_read_byte((unsigned char *) address);
}

uint16_t EEPROMReader::readInt(byte address)
{
  TwoBytes fetcher;
  fetcher.array[0] = readByte(address);
  fetcher.array[1] = readByte(address+1);
  return fetcher.integer;
}
uint32_t EEPROMReader::readLong(byte address)
{
  FourBytes fetcher;
  fetcher.array[0] = readInt(address);
  fetcher.array[1] = readInt(address+2);
  return fetcher.integer;
}

void EEPROMReader::readString(char* string, byte len, byte address)
{
  for(byte i = 0; i < len; i++) {
    string[i] = readByte(address++);
  }
}

void EEPROMReader::writeByte(uint8_t val, byte address)
{
  return eeprom_write_byte((unsigned char *) address, val);
}

void EEPROMReader::writeInt(uint16_t val, byte address)
{
  TwoBytes storer = {val};
  writeByte(storer.array[0], address);
  writeByte(storer.array[1], address+1);
}
void EEPROMReader::writeLong(uint32_t val, byte address)
{
  FourBytes storer = {val};
  writeInt(storer.array[0], address);
  writeInt(storer.array[1], address+2);
}
void EEPROMReader::writeString(const char* string, byte len, byte address)
{
  for(byte i = 0; i < len; i++) {
    writeByte(string[i], address+i);
  }
}

EEPROMReader EEPROM;

