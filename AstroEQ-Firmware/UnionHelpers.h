
#ifndef __UNION_HELPERS_H__
#define __UNION_HELPERS_H__

#include <inttypes.h>
typedef uint8_t byte;

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

typedef union {
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
} Inter;

inline Inter InterMaker(byte _top, byte _high, byte _mid, byte _low){
	Inter inter;
	inter.low = _low;
	inter.mid = _mid;
	inter.high = _high;
	inter.top = _top;
	return inter;
}

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

#endif //__UNION_HELPERS_H__
