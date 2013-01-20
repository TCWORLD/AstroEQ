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
void Commands::init(unsigned long eVal,unsigned long aVal1,unsigned long aVal2,unsigned long bVal1,unsigned long bVal2,unsigned long sVal1,unsigned long sVal2,byte gVal){
  for(byte i = 0;i < 2;i++){
    _flag[i] = 0x100;
    _jVal[i] = 0x800000; //Current position, 0x800000 is the centre
    _IVal[i] = 0; //Recieved Speed
    _GVal[i] = 0; //Mode recieved from :G command
    _HVal[i] = 0; //Value recieved from :H command
    _eVal[i] = eVal; //version number
    _gVal[i] = gVal; //High speed scalar
  }
  _aVal[0] = aVal1; //steps/axis
  _aVal[1] = aVal2; //steps/axis
  _bVal[0] = bVal1; //sidereal rate
  _bVal[1] = bVal2; //sidereal rate
  _sVal[0] = sVal1; //steps/worm rotation
  _sVal[1] = sVal2; //steps/worm rotation
}
      
void Commands::init(unsigned long eVal,unsigned long aVal,unsigned long bVal,byte gVal,unsigned long sVal){ 
  for(byte i = 0;i < 2;i++){
    _flag[i] = 0x100;
    _jVal[i] = 0x800000; //Current position, 0x800000 is the centre
    _IVal[i] = 0; //Recieved Speed
    _GVal[i] = 0; //Mode recieved from :G command
    _HVal[i] = 0; //Value recieved from :H command
    _eVal[i] = eVal; //version number
    _aVal[i] = aVal; //steps/axis
    _bVal[i] = bVal; //sidereal rate
    _gVal[i] = gVal; //High speed scalar
    _sVal[i] = sVal; //steps/worm rotation
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
                                                      {'L', 0, 0} };

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

unsigned int Commands::fVal(byte target){
  return _flag[target];
}

char Commands::stepDir(byte target){ //Get Method
  char stepDir = 1 - ((_flag[target] >> 8) & 0x02); //get step direction
  return stepDir;
}

byte Commands::dir(byte target, char dir){ //Set Method
  if(dir == 1){
    //set direction
    _flag[target] |= (1 << 9); //set bit
  } else if (dir == 0){
    _flag[target] &= ~(1 << 9); //unset bit
  } else {
    dir = (_flag[target] >> 9) & 0x01; //get direction
  }
  return dir;
}

byte Commands::stopped(byte target, byte stopped){ //Set Method
  if(stopped == 1){
    _flag[target] |= (1 << 8); //set bit
  } else if (stopped == 0){
    _flag[target] &= ~(1 << 8); //unset bit
  } else {
    stopped = (_flag[target] >> 8) & 0x01;
  }
  return stopped;
}

byte Commands::gotoEn(byte target, byte gotoEn){ //Set Method
  if(gotoEn == 1){
    _flag[target] |= (1 << 4); //set bit
  } else if (gotoEn == 0){
    _flag[target] &= ~(1 << 4); //unset bit
  } else {
    gotoEn = (_flag[target] >> 4) & 0x01;
  }
  return gotoEn;
}

byte Commands::FVal(byte target, byte FVal){ //Set Method
  if(FVal == 1){
    _flag[target] |= (1 << 0); //set bit
  } else if (FVal == 0){
    _flag[target] &= ~(1 << 0); //unset bit
  } else {
    FVal = (_flag[target] >> 0) & 0x01;
  }
  return FVal;
}

unsigned long Commands::jVal(byte target, unsigned long jVal){ //Set Method
  if(jVal < 0x01000000){
    _jVal[target] = jVal;
  } else {
    jVal = _jVal[target];
  }
  return jVal;
}

unsigned long Commands::IVal(byte target, unsigned long IVal){ //Set Method
  if(IVal < 0x01000000){
    _IVal[target] = IVal;
  } else {
    IVal = _IVal[target];
  }
  return IVal;
}

byte Commands::GVal(byte target, byte GVal){ //Set Method
  if(GVal < 4){
    _GVal[target] = GVal;
  } else {
    GVal = _GVal[target];
  }
  return GVal;
}

unsigned long Commands::HVal(byte target, unsigned long HVal){ //Set Method
  if(HVal < 0x01000000){
    _HVal[target] = HVal;
  } else {
    HVal = _HVal[target];
  }
  return HVal;
}

unsigned long Commands::eVal(byte target){
  return _eVal[target];
}

unsigned long Commands::aVal(byte target){
  return _aVal[target];
}

unsigned long Commands::bVal(byte target){
  return _bVal[target];
}

byte Commands::gVal(byte target){
  return _gVal[target];
}

unsigned long Commands::sVal(byte target){
  return _sVal[target];
}
