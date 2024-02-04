
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

#endif //__UNION_HELPERS_H__
