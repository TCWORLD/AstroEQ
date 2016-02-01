
#ifndef synta_h
#define synta_h

#include "AstroEQ.h"
#include "commands.h"

void synta_initialise(unsigned long version, byte gVal);
void synta_assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData);
char synta_recieveCommand(char* dataPacket, char character);
byte synta_axis(byte axis = 2); //make target readonly to outside world.
char synta_command(); //make current command readonly to outside world.
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
  
#endif
