
#ifndef __EEPROM_ADDRESSES_H__
#define __EEPROM_ADDRESSES_H__

#include "avr/io.h"

#define EEPROMStart_Address (                      0 )
#define AstroEQID_Address   (EEPROMStart_Address + 0 )
#define AstroEQVer_Address  (EEPROMStart_Address + 3 )
#define AstroEQCRC_Address  (EEPROMStart_Address + 7 )
#define Microstep_Address   (EEPROMStart_Address + 8 ) //whether to use microstepping.
#define RAReverse_Address   (EEPROMStart_Address + 9 )
#define DECReverse_Address  (EEPROMStart_Address + 10)
#define Driver_Address      (EEPROMStart_Address + 11)
#define RAGoto_Address      (EEPROMStart_Address + 12)
#define DECGoto_Address     (EEPROMStart_Address + 13)
#define aVal1_Address       (EEPROMStart_Address + 14) //steps/axis
#define aVal2_Address       (EEPROMStart_Address + 18) //steps/axis
#define bVal1_Address       (EEPROMStart_Address + 22) //sidereal rate
#define bVal2_Address       (EEPROMStart_Address + 26) //sidereal rate
#define sVal1_Address       (EEPROMStart_Address + 30) //steps/worm rotation
#define sVal2_Address       (EEPROMStart_Address + 34) //steps/worm rotation
#define IVal1_Address       (EEPROMStart_Address + 38) //steps/worm rotation
#define IVal2_Address       (EEPROMStart_Address + 40) //steps/worm rotation
#define GearEnable_Address  (EEPROMStart_Address + 42) //Allow "gear change"
#define AdvHCEnable_Address (EEPROMStart_Address + 43) //Allow advanced controller detection
#define DecBacklash_Address (EEPROMStart_Address + 44) //DEC backlash correction factor
#define SpeedFactor_Address (EEPROMStart_Address + 46) //ST4 Speed Factor (0.05x to 0.95x sidereal as multiple of 1/20)
#define SnapPinOD_Address   (EEPROMStart_Address + 47)

#define AccelTableLength 64
#define AccelTable1_Address (EEPROMStart_Address + 100) //Leave a gap so we can add more settings later.
#define AccelTable2_Address (EEPROMStart_Address + 100 + AccelTableLength*3)

#define EEPROMEnd_Address   (EEPROMStart_Address + 100 + (6*AccelTableLength) - 1)

#if (EEPROMEnd_Address > E2END)
    #error "AccelTable too large for EEPROM"
#endif

#endif //__EEPROM_ADDRESSES_H__
