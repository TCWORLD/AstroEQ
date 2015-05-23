//command Structures ---------------------------------------------------------
//
// Definition of the commands used by the Synta protocol, and variables in which responses
// are storedz
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

#include "commands.h"

Commands cmd = {0};

void Commands_init(unsigned long _eVal, byte _gVal){
  cmd.aVal[RA] = EEPROM_readLong(aVal1_Address); //steps/axis
  cmd.aVal[DC] = EEPROM_readLong(aVal2_Address); //steps/axis
  cmd.bVal[RA] = EEPROM_readLong(bVal1_Address); //sidereal rate
  cmd.bVal[DC] = EEPROM_readLong(bVal2_Address); //sidereal rate
  cmd.sVal[RA] = EEPROM_readLong(sVal1_Address); //steps/worm rotation
  cmd.sVal[DC] = EEPROM_readLong(sVal2_Address); //steps/worm rotation
  cmd.siderealIVal[RA] = EEPROM_readInt(IVal1_Address); //steps/worm rotation
  cmd.siderealIVal[DC] = EEPROM_readInt(IVal2_Address); //steps/worm rotation
  cmd.normalGotoSpeed[RA] = EEPROM_readByte(RAGoto_Address); //IVal for normal goto speed
  cmd.normalGotoSpeed[DC] = EEPROM_readByte(DECGoto_Address); //IVal for normal goto speed
  for(byte i = 0;i < 2;i++){
    cmd.dir[i] = 0;
    cmd.stepDir[i] = 1; //1-dir*2
    cmd.stopped[i] = 1;
    cmd.gotoEn[i] = 0;
    cmd.FVal[i] = 0;
    cmd.jVal[i] = 0x800000; //Current position, 0x800000 is the centre
    cmd.IVal[i] = cmd.siderealIVal[i]; //Recieved Speed will be set by :I command.
    cmd.GVal[i] = 0; //Mode recieved from :G command
    cmd.HVal[i] = 0; //Value recieved from :H command
    cmd.eVal[i] = _eVal; //version number
    cmd.gVal[i] = _gVal; //High speed scalar
    cmd.minSpeed[i] = cmd.siderealIVal[i]>>1;//2x sidereal rate. [minspeed is the point at which acceleration curves are enabled]
    cmd.stopSpeed[i] = cmd.minSpeed[i];
    cmd.currentIVal[i] = cmd.stopSpeed[i]+1; //just slower than stop speed as axes are stopped.
    cmd.motorSpeed[i] = cmd.stopSpeed[i]+1; //same as above.
    cmd.stepRepeat[i] = 0;//siderealIVal[i]/75;//((aVal[i] < 5600000UL) ? ((aVal[i] < 2800000UL) ? 16 : 8) : 4);
  }
}

const char cmd_commands[numberOfCommands][3] = { {'j', 0, 6}, //arranged in order of most frequently used to reduce searching time.
                                                 {'f', 0, 3},
                                                 {'I', 6, 0},
                                                 {'G', 2, 0},
                                                 {'J', 0, 0},
                                                 {'K', 0, 0},
                                                 {'H', 6, 0},
                                                 {'M', 6, 0},
                                                 {'e', 0, 6},
                                                 {'a', 0, 6},
                                                 {'b', 0, 6},
                                                 {'g', 0, 2},
                                                 {'s', 0, 6},
                                                 {'E', 6, 0},
                                                 {'P', 1, 0},
                                                 {'F', 0, 0},
                                                 {'L', 0, 0},
                                                 //Programmer Commands
                                                 {'A', 6, 0},
                                                 {'B', 6, 0},
                                                 {'S', 6, 0},
                                                 {'n', 0, 6},
                                                 {'N', 6, 0},
                                                 {'D', 2, 0},
                                                 {'d', 0, 2},
                                                 {'C', 1, 0},
                                                 {'c', 0, 2},
                                                 {'Z', 2, 0},
                                                 {'z', 0, 2},
                                                 {'O', 1, 0},
                                                 {'T', 0, 0},
                                                 {'R', 0, 0}
                                               };

char Commands_getLength(char cmd, bool sendRecieve){
  for(byte i = 0;i < numberOfCommands;i++){
    if(cmd_commands[i][0] == cmd){
      if(sendRecieve){
        return cmd_commands[i][1];
      } else {
        return cmd_commands[i][2];
      }
    }
  }
  return -1;
}

//void Commands::setStepLength(byte target, byte stepLength) {
//  if (stepDir[target] > 0) {
//    stepDir[target] = stepLength;
//  } else {
//    stepDir[target] = -stepLength;
//  }
//}

