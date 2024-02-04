
#ifndef __SYNTA_H__
#define __SYNTA_H__

#include "AstroEQ.h"
#include "commands.h"

enum __attribute__((packed)) {
    PACKET_ERROR_BADCHAR = -2,
    PACKET_ERROR_GENERAL = -1,
    PACKET_ERROR_PARTIAL = 0
};

typedef enum __attribute__((packed)) {
    SYNTA_NOERROR = -1,
    SYNTA_ERROR_UNKNOWNCMD = 0,
    SYNTA_ERROR_CMDLENGTH,
    SYNTA_ERROR_NOSTOPPED,
    SYNTA_ERROR_INVALIDCHAR,
    SYNTA_ERROR_NOTINIT,
    SYNTA_ERROR_DRIVERSLEEP
} SyntaError;

void synta_initialise(unsigned long version, byte gVal);
void synta_assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData, CmdProgMode isProg);
char synta_recieveCommand(char* dataPacket, char character, CmdProgMode isProg);
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
