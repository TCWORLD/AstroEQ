
#ifndef synta_h
#define synta_h
  #include "AstroEQ.h"
  #include "commands.h"
  
  void synta_initialise(unsigned long version);
  void synta_assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData);
  char synta_recieveCommand(char* dataPacket, char character);
  byte synta_axis(byte axis = 2); //make target readonly to outside world.
  char synta_command(); //make current command readonly to outside world.
  unsigned long synta_hexToLong(char* hex);
  byte synta_hexToByte(char* hex);
  
  //Methods for accessing command variables
  extern inline void cmd_setDir(byte target, byte _dir);
  extern inline void cmd_updateStepDir(byte target, byte stepSize);
  extern inline unsigned int cmd_fVal(byte target);
  extern inline void cmd_setStopped(byte target, byte _stopped);
  extern inline void cmd_setGotoEn(byte target, byte _gotoEn);
  extern inline void cmd_setFVal(byte target, byte _FVal);
  extern inline void cmd_setjVal(byte target, unsigned long _jVal);
  extern inline void cmd_setIVal(byte target, unsigned int _IVal);
  extern inline void cmd_setHVal(byte target, unsigned long _HVal);
  extern inline void cmd_setGVal(byte target, byte _GVal);
  
#endif
