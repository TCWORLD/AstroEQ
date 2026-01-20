/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Synta
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts, and also a great many custom mount configurations.
 
  Current Version: <See "../VerNum.txt">
*/

//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#ifndef __ASTROEQ_H__
#define __ASTROEQ_H__

//Define the version number
#define ASTROEQ_VER     astroEQ_vernum()
#define ASTROEQ_VER_STR astroEQ_verstr()

#ifdef __cplusplus
extern "C"{
#endif

/*
 * File Includes
 */
 
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

#ifndef __cplusplus
#include <stdbool.h>
#endif

/*
 * Version number helper inline
 */

inline unsigned long astroEQ_vernum () {
    //This function will grab the version number from VerNum.txt which will be
    //a floating point integer to 2.d.p. The result is then multiplied by 100
    //to convert to an integer version number representation. The result is then
    //returned. The whole operation will be optimised to a constant.
    return (unsigned long)(100.0 *
    #include "../VerNum.txt"
    );
}

inline const char* astroEQ_verstr() {
    //This function will grab the version number from VerNumStr.txt which will be
    //a stringified version of the VerNum.txt file produced by the prebuild event.
    //The whole operation will be optimised to a constant.
    return 
    #include "../VerNumStr.txt"
    ;
}

inline bool astroEQ_verlessthan(unsigned long verReq) {
    return (verReq < astroEQ_vernum());
}

/*
 * Useful Defines
 */
 
#ifndef sbi
  #define sbi(r,b) r |= _BV(b)
#endif
#ifndef cbi
  #define cbi(r,b) r &= ~_BV(b)
#endif

#ifndef _BV
  #define _BV(x) (1 << x)
#endif

#define FLOAT (1<<3)
#define HIGH 1
#define LOW 0

enum __attribute__((packed)) {
    INPUT = 0,
    OUTPUT
};

enum __attribute__((packed)) {
    A498x = 0,
    DRV882x,
    DRV8834,
    TMC2100,
    DRIVER_MAX = TMC2100
};

enum __attribute__((packed)) {
    SPEEDNORM = 0,
    SPEEDFAST
};

enum __attribute__ ((packed)) {
    STOPNORMAL = false,
    STOPEMERGENCY = true
};

typedef enum __attribute__((packed)) {
    RUNMODE = 0,
    PROGMODE,
    STOREMODE,
    REBUILDMODE
} ExecMode;

#ifdef abs
#undef abs
#endif

#define abs(x) ((x)>0?(x):-(x))

#ifndef max
#define max(a,b) ((a > b) ? a : b)
#endif

typedef enum __attribute__((packed)) {
    ST4P = 0, //Positive ST4 Pin
    ST4N = 1, //Negative ST4 Pin
    ST4O = -1 //Neither ST4 Pin
} ST4Pin;

typedef enum __attribute__((packed)) {
    MOTION_START_NOTREADY = 0,
    MOTION_START_REQUESTED,
    MOTION_START_UPDATABLE
} MotionStart;

typedef enum __attribute__((packed)){
    CMD_ST4_DEFAULT = 0,
    CMD_ST4_STANDALONE,
    CMD_ST4_HIGHSPEED,
    CMD_ST4_EQMOD,
    CMD_ST4_LUNAR,
    CMD_ST4_SOLAR
} ST4SpeedMode;

typedef enum __attribute__((packed)){
    CMD_ST4_EQMOD_1X = 0,
    CMD_ST4_EQMOD_p75X,
    CMD_ST4_EQMOD_p5X,
    CMD_ST4_EQMOD_p25X,
    CMD_ST4_EQMOD_p125X,
    CMD_ST4_EQMOD_COUNT
} ST4EqmodSpeed;

typedef enum __attribute__((packed)) {
    EQMOD_MODE = 0,
    BASIC_HC_MODE,
    ADVANCED_HC_MODE
} CommsMode;

typedef enum __attribute__((packed)){
    RA, //Right Ascension is AstroEQ axis 0 (Synta axis '1')
    DC, //Declination is AstroEQ axis 1 (Synta axis '2')
    AXIS_COUNT
} MotorAxis;

#define MIN_IVAL 50
#define MAX_IVAL 1200

#define BAUD_RATE 9600

#define nop() __asm__ __volatile__ ("nop \n\t")

/*
 * Standalone Pin Names
 */

#define STANDALONE_IRQ   0
#define STANDALONE_PULL  1
#define STANDALONE_TGT   2

typedef struct {
    unsigned int speed;
    byte repeats;
} AccelTableStruct;

/*
 * Declare constant arrays of pin numbers
 */
#ifndef gpioPin_3_Define
#define gpioPin_3_Define 255
#else
#define TARGET_SPEED_GPIO_PIN
#endif
static const byte standalonePin[3] = {gpioPin_0_Define,gpioPin_2_Define,gpioPin_3_Define};
static const byte snapPin = gpioPin_1_Define;
static const byte pwmPin = pwmPin_Define;
#ifdef statusPinShadow_Define
static const byte statusPinShdw = statusPinShadow_Define;
#endif
static const byte statusPin = statusPin_Define;
#ifdef estopPin_Define
static const byte estopPin = estopPin_Define;
#endif
static const byte resetPin[2] = {resetPin_0_Define,resetPin_1_Define};
static const byte dirPin[2] = {dirPin_0_Define,dirPin_1_Define};
static const byte enablePin[2] = {enablePin_0_Define,enablePin_1_Define};
static const byte stepPin[2] = {stepPin_0_Define,stepPin_1_Define};
static const byte st4Pins[2][2] = {{ST4AddPin_0_Define,ST4SubPin_0_Define},{ST4AddPin_1_Define,ST4SubPin_1_Define}};
static const byte modePins[2][3] = {{modePins0_0_Define,modePins1_0_Define,modePins2_0_Define},{modePins0_1_Define,modePins1_1_Define,modePins2_1_Define}};


/*
 * Function Prototypes
 */
 
bool checkEEPROMCRC();
unsigned char calculateEEPROMCRC();
bool checkEEPROM(bool skipCRC);
void buildEEPROM();
void storeEEPROM();
void systemInitialiser();
CommsMode standaloneModeTest();
ST4SpeedMode checkBasicHCSpeed();
int main(void);
bool decodeCommand(char command, char* packetIn);
void calculateRate(byte axis);
void calculateDecelerationLength (byte axis);
void motorEnable(byte axis);
void motorDisable(byte axis);
void slewMode(byte axis);
void gotoMode(byte axis);
void motorStart(byte motor);
void motorStartRA();
void motorStartDC();
void motorStop(byte motor, byte emergency);
void motorStopRA(bool emergency);
void motorStopDC(bool emergency);
void configureTimer();
void buildModeMapping(byte microsteps, byte driverVersion);


#ifdef __cplusplus
} // extern "C"
#endif


#endif //__ASTROEQ_H__


#else
#error Unsupported Part! Please use an Arduino Mega, or ATMega162
#endif
