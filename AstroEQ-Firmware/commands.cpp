//command Structures ---------------------------------------------------------
//
// Definition of the commands used by the Synta protocol, and variables in which responses
// are stored
//
// Data structure of flag Flag:
//   flag = xxxx00ds000g000f where bits:
//   x = dont care
//   d = dir
//   s = stopped
//   g = goto
//   f = energised
//
// Only dir can be used to set the direction, but stepDir method can be used
// to returns it in a more useful format
//
//----------------------------------------------------------------------------

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "commands.h"
void Commands::init(unsigned long _eVal, byte _gVal){
  aVal[0] = EEPROM.readLong(aVal1_Address); //steps/axis
  aVal[1] = EEPROM.readLong(aVal2_Address); //steps/axis
  bVal[0] = EEPROM.readLong(bVal1_Address); //sidereal rate
  bVal[1] = EEPROM.readLong(bVal2_Address); //sidereal rate
  sVal[0] = EEPROM.readLong(sVal1_Address); //steps/worm rotation
  sVal[1] = EEPROM.readLong(sVal2_Address); //steps/worm rotation
  siderealIVal[0] = EEPROM.readInt(IVal1_Address); //steps/worm rotation
  siderealIVal[1] = EEPROM.readInt(IVal2_Address); //steps/worm rotation
  for(byte i = 0;i < 2;i++){
    dir[i] = 0;
    stopped[i] = 1;
    gotoEn[i] = 0;
    FVal[i] = 0;
    jVal[i] = 0x800000; //Current position, 0x800000 is the centre
    IVal[i] = 0; //Recieved Speed
    GVal[i] = 0; //Mode recieved from :G command
    HVal[i] = 0; //Value recieved from :H command
    eVal[i] = _eVal; //version number
    gVal[i] = _gVal; //High speed scalar
    
    stepIncrement[i] = siderealIVal[i]/75;//((aVal[i] < 5600000UL) ? ((aVal[i] < 2800000UL) ? 16 : 8) : 4);
  }
}

const char Commands::command[numberOfCommands][3] = { {'e', 0, 6},
                                                      {'a', 0, 6},
                                                      {'b', 0, 6},
                                                      {'g', 0, 2},
                                                      {'s', 0, 6},
                                                      {'K', 0, 0},
                                                      {'E', 6, 0},
                                                      {'j', 0, 6},
                                                      {'f', 0, 3},
                                                      {'G', 2, 0},
                                                      {'H', 6, 0},
                                                      {'M', 6, 0},
                                                      {'I', 6, 0},
                                                      {'J', 0, 0},
                                                      {'P', 1, 0},
                                                      {'F', 0, 0},
                                                      {'L', 0, 0},
                                                      {'A', 6, 0},
                                                      {'B', 6, 0},
                                                      {'S', 6, 0} };

char Commands::getLength(char cmd, boolean sendRecieve){
  for(byte i = 0;i < numberOfCommands;i++){
    if(command[i][0] == cmd){
      if(sendRecieve){
        return command[i][1];
      } else {
        return command[i][2];
      }
    }
  }
  return -1;
}

void Commands::setStepLength(byte target, byte stepLength) {
  if (stepDir[target] > 0) {
    stepDir[target] = stepLength;
  } else {
    stepDir[target] = -stepLength;
  }
}

void Commands::setDir(byte target, byte _dir){ //Set Method
  _dir &= 1;
  //byte sign = _dir ^ target;
  //if (sign & 1){
  if(_dir){
    stepDir[target] = -1; //set step direction
  } else {//if (dir == 0){}
    stepDir[target] = 1; //set step direction
  }
  dir[target] = _dir; //set direction
}

void Commands::setStopped(byte target, byte _stopped){ //Set Method
  stopped[target] = _stopped & 1;
}

void Commands::setGotoEn(byte target, byte _gotoEn){ //Set Method
  gotoEn[target] = _gotoEn & 1;
}

void Commands::setFVal(byte target, byte _FVal){ //Set Method
  FVal[target] = _FVal & 1;
}

unsigned int Commands::fVal(byte target){ //_fVal: 00ds000g000f
  return ((dir[target] << 9)|(stopped[target] << 8)|(gotoEn[target] << 4)|(FVal[target] << 0));
}

void Commands::setjVal(byte target, unsigned long _jVal){ //Set Method
  jVal[target] = _jVal;
}

void Commands::setIVal(byte target, unsigned int _IVal){ //Set Method
  IVal[target] = _IVal;
}

void Commands::setHVal(byte target, unsigned long _HVal){ //Set Method
  HVal[target] = _HVal;
}

void Commands::setGVal(byte target, byte _GVal){ //Set Method
  GVal[target] = _GVal;
}
