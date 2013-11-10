//_fVal Flag get/set callers -------------------------------------------------
//
//Data structure of _fVal Flag:
//  _fVal = xxxx00ds000g000f where bits:
//  x = dont care
//  d = dir
//  s = stopped
//  g = goto
//  f = energised
//
//Only dir can be used to set the direction, but stepDir method can be used
//to returns it in a more useful format
//
//----------------------------------------------------------------------------
#ifndef commands_h
#define commands_h
  
  #if ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
  
  #include "EEPROMReader.h"
  #include "EEPROMAddresses.h"
  
  #define numberOfCommands 20
    
  class Commands{
    public:
      
      void init(unsigned long _eVal, byte _gVal);
      
      //Command definitions
      static const char command[numberOfCommands][3];
      
      //Methods for accessing class variables
      //void setStepLength(byte target, byte stepLength); //in highspeed mode, one step is gVal increments of the jVal.
      void updateStepDir(byte target); //Update current direction to match required.
      void setDir(byte target, byte _dir); //Set Method
      void setStopped(byte target, byte _stopped); //Set Method
      void setGotoEn(byte target, byte _gotoEn); //Set Method
      void setFVal(byte target, byte _FVal); //Set Method
      void setjVal(byte target, unsigned long _jVal); //Set Method
      void setIVal(byte target, unsigned int _IVal); //Set Method
      void setHVal(byte target, unsigned long _HVal); //Set Method
      void setGVal(byte target, byte _GVal); //Set Method
      
      char getLength(char cmd, boolean sendRecieve);
      
      //class variables
      unsigned long jVal[2]; //_jVal: Current position
      unsigned int IVal[2]; //_IVal: speed to move if in slew mode
      unsigned int motorSpeed[2]; //speed at which moving. Accelerates to IVal.
      byte GVal[2]; //_GVal: slew/goto mode
      unsigned long HVal[2]; //_HVal: steps to move if in goto mode
      unsigned int fVal(byte target); //_fVal: 00ds000g000f; d = dir, s = stopped, g = goto, f = energised
      volatile char stepDir[2]; 
      byte dir[2];
      byte FVal[2];
      byte gotoEn[2];
      byte stopped[2];
      unsigned long eVal[2]; //_eVal: Version number
      unsigned long aVal[2]; //_aVal: Steps per axis revolution
      unsigned long bVal[2]; //_bVal: Sidereal Rate of axis
      byte gVal[2]; //_gVal: Speed scalar for highspeed slew
      unsigned long sVal[2]; //_sVal: Steps per worm gear revolution
      byte stepRepeat[2];
      unsigned int siderealIVal[2]; //_IVal: at sidereal rate
      unsigned int currentIVal[2]; //_IVal: this will be upldated to match the requested IVal once the motors are stopped.
              
  };
#endif
