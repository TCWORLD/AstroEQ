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
  
  #define numberOfCommands 17
  
  class Commands{
    public:
      void init(unsigned long eVal,unsigned long aVal1,unsigned long aVal2,unsigned long bVal1,unsigned long bVal2,unsigned long sVal1,unsigned long sVal2,byte gVal);
      void init(unsigned long eVal,unsigned long aVal,unsigned long bVal,byte gVal,unsigned long sVal);
      
      //Command definitions
      static const char command[numberOfCommands][3];
      
      //Methods for accessing class variables
      char stepDir(byte target); //Get Method
      byte dir(byte target, char dir = 2); //Get Method
      byte stopped(byte target, byte stopped = 2); //Get Method
      byte gotoEn(byte target, byte gotoEn = 2); //Get Method
      byte FVal(byte target, byte FVal = 2); //Get Method
      unsigned int fVal(byte target); //return the flag to main program
      unsigned long jVal(byte target, unsigned long jVal = 0x01000000); //Get Method
      unsigned long IVal(byte target, unsigned long IVal = 0x01000000); //Get Method
      byte GVal(byte target, byte GVal = 4); //Get Method
      unsigned long HVal(byte target, unsigned long HVal = 0x01000000); //Get Method
      unsigned long eVal(byte target); //Get Method
      unsigned long aVal(byte target); //Get Method
      unsigned long bVal(byte target); //Get Method
      byte gVal(byte target); //Get Method
      unsigned long sVal(byte target); //Get Method
      
      char getLength(char cmd, boolean sendRecieve);
      
    private:
      //class variables
      unsigned long _jVal[2]; //_jVal: Current position
      unsigned long _IVal[2]; //_IVal: speed to move if in slew mode
      byte _GVal[2]; //_GVal: slew/goto mode
      unsigned long _HVal[2]; //_HVal: steps to move if in goto mode
      unsigned int _flag[2]; //_fVal: 00ds000g000f; d = dir, s = stopped, g = goto, f = energised
      unsigned long _eVal[2]; //_eVal: Version number
      unsigned long _aVal[2]; //_aVal: Steps per axis revolution
      unsigned long _bVal[2]; //_bVal: Sidereal Rate of axis
      byte _gVal[2]; //_gVal: Speed scalar for highspeed slew
      unsigned long _sVal[2]; //_sVal: Steps per worm gear revolution
  };
#endif
