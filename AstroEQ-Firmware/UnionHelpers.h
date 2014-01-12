
#ifndef UnionHelpers_h
#define UnionHelpers_h

typedef union {
  uint16_t integer;
  uint8_t array[2];
} TwoBytes;

typedef union {
  uint32_t integer;
  uint16_t array[2];
  uint8_t bytes[4];
} FourBytes;

typedef union{
  byte integer;
  struct {
    byte low:4;
    byte high:4;
  };
} Nibbler;

typedef union{
  unsigned int integer;
  struct {
    byte low;
    byte high;
  };
  struct {
    Nibbler lowNibbler;
    Nibbler highNibbler;
  };
} Byter;

typedef union InterMaker{
  unsigned long integer;
  struct {
    byte low;
    byte mid;
    byte high;
    byte top;
  };
  struct {
    Byter lowByter;
    Byter highByter;
  };
  InterMaker(unsigned long _integer){
    integer = _integer;
  }
  InterMaker(byte _top, byte _high, byte _mid, byte _low){
    low = _low;
    mid = _mid;
    high = _high;
    top = _top;
  }
} Inter;

typedef union{
  unsigned int integer;
  struct {
    unsigned int low:4;
    unsigned int mid:4;
    unsigned int high:4;
    unsigned int:4;
  };
  struct {
    Nibbler lowNibbler;
    Nibbler highNibbler;
  };
} DoubleNibbler;

#endif
