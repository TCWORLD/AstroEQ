#ifndef EEPROM_h
#define EEPROM_h

#include "Arduino.h"
#include <inttypes.h>

typedef union {
  uint16_t integer;
  uint8_t array[2];
} TwoBytes;

typedef union {
  uint32_t integer;
  uint16_t array[2];
  uint8_t bytes[4];
} FourBytes;

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
