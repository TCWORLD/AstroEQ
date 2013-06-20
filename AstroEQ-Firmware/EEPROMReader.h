#ifndef EEPROM_h
#define EEPROM_h

#include "Arduino.h"
#include "UnionHelpers.h"

class EEPROMReader
{
  public:
    uint8_t readByte(byte);
    uint16_t readInt(byte);
    uint32_t readLong(byte);
    void readString(char* string, byte len, byte address);
    void writeByte(uint8_t,byte);
    void writeInt(uint16_t,byte);
    void writeLong(uint32_t,byte);
    void writeString(const char* string, byte len, byte address);
};

extern EEPROMReader EEPROM;

#endif
