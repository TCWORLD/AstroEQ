
#ifndef __SYNTA_H__
#define __SYNTA_H__

#include "AstroEQ.h"

#define SYNTA_ERROR_UNKNOWNCMD  0
#define SYNTA_ERROR_CMDLENGTH   1
#define SYNTA_ERROR_NOSTOPPED   2
#define SYNTA_ERROR_INVALIDCHAR 3
#define SYNTA_ERROR_NOTINIT     4
#define SYNTA_ERROR_DRIVERSLEEP 5

void synta_initialise(unsigned long version, byte gVal);
void synta_assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData, bool isProg);
char synta_recieveCommand(char* dataPacket, char character, bool isProg);
byte synta_setaxis(byte axis);
byte synta_getaxis();
char synta_command();
unsigned long synta_hexToLong(char* hex);
byte synta_hexToByte(char* hex);

//Methods for accessing command variables - now in commands.h
//void cmd_setDir(byte target, byte _dir);
//void cmd_updateStepDir(byte target, byte stepSize);
//unsigned int cmd_fVal(byte target);
//void cmd_setStopped(byte target, byte _stopped);
//void cmd_setGotoEn(byte target, byte _gotoEn);
//void cmd_setFVal(byte target, byte _FVal);
//void cmd_setjVal(byte target, unsigned long _jVal);
//void cmd_setIVal(byte target, unsigned int _IVal);
//void cmd_setHVal(byte target, unsigned long _HVal);
//void cmd_setGVal(byte target, byte _GVal);
  
#endif //__SYNTA_H__
