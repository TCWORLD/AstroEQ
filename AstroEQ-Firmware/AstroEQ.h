/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Synta
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts, and also a great many custom mount configurations.
 
  Current Verison: 7.5
*/

//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#ifndef __ASTROEQ_H__
#define __ASTROEQ_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "PinMappings.h" //Read Pin Mappings
#include "EEPROMAddresses.h" //Config file addresses
#include "UnionHelpers.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <inttypes.h>
#ifndef sbi
  #define sbi(r,b) r |= _BV(b)
#endif
#ifndef cbi
  #define cbi(r,b) r &= ~_BV(b)
#endif

#define FLOAT (1<<3)
#define HIGH 1
#define LOW 0

#define OUTPUT 1
#define INPUT 0

#define A498x 0
#define DRV882x 1
#define DRV8834 2

#define REBUILDMODE 3
#define STOREMODE 2
#define PROGMODE 1
#define RUNMODE 0

#ifdef abs
#undef abs
#endif

#define abs(x) ((x)>0?(x):-(x))

#ifndef max
#define max(a,b) ((a > b) ? a : b)
#endif

typedef uint8_t byte;

#define RA 0 //Right Ascension is AstroEQ axis 0 (Synta axis '1')
#define DC 1 //Declination is AstroEQ axis 1 (Synta axis '2')

#define MIN_IVAL 50

bool checkEEPROM();
void buildEEPROM();
void storeEEPROM();
void systemInitialiser();
unsigned int calculateDecelerationLength (byte axis);
int main(void);
void decodeCommand(char command, char* packetIn);
void calculateRate(byte motor);
void motorEnable(byte axis);
void motorDisable(byte axis);
void slewMode(byte axis);
void gotoMode(byte axis);
void motorStart(byte motor);
void motorStartRA();
void motorStartDC();
void motorStop(byte motor, byte emergency);
void motorStopRA(byte emergency);
void motorStopDC(byte emergency);
void configureTimer();
void buildModeMapping(byte microsteps, byte driverVersion);


#ifdef __cplusplus
} // extern "C"
#endif


#endif


#else
#error Unsupported Part! Please use an Arduino Mega, or ATMega162
#endif
