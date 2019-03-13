//Command Structures ---------------------------------------------------------
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
    cmd.aVal[RA] = EEPROM_readLong(aVal1_Address);              //steps/axis
    cmd.aVal[DC] = EEPROM_readLong(aVal2_Address);              //steps/axis
    cmd.bVal[RA] = EEPROM_readLong(bVal1_Address);              //sidereal rate
    cmd.bVal[DC] = EEPROM_readLong(bVal2_Address);              //sidereal rate
    cmd.sVal[RA] = EEPROM_readLong(sVal1_Address);              //steps/worm rotation
    cmd.sVal[DC] = EEPROM_readLong(sVal2_Address);              //steps/worm rotation
    
    cmd.siderealIVal[RA] = EEPROM_readInt(IVal1_Address);       //steps/worm rotation
    cmd.siderealIVal[DC] = EEPROM_readInt(IVal2_Address);       //steps/worm rotation
    cmd.normalGotoSpeed[RA] = EEPROM_readByte(RAGoto_Address);  //IVal for normal goto speed
    cmd.normalGotoSpeed[DC] = EEPROM_readByte(DECGoto_Address); //IVal for normal goto speed
    cmd.st4SpeedFactor = EEPROM_readByte(SpeedFactor_Address);  //ST4 speed factor
    cmd.st4DecBacklash = EEPROM_readInt(DecBacklash_Address);   //DEC backlash steps
    
    EEPROM_readAccelTable(cmd.accelTable[RA],AccelTableLength,AccelTable1_Address); //Load the RA accel/decel table
    EEPROM_readAccelTable(cmd.accelTable[DC],AccelTableLength,AccelTable2_Address); //Load the DC accel/decel table
    
    for(byte i = 0;i < 2;i++){
        cmd.dir[i] = CMD_FORWARD;
        cmd.stepDir[i] = 1; //1-dir*2
        cmd.highSpeedMode[i] = false;
        cmd.stopped[i] = CMD_STOPPED;
        cmd.gotoEn[i] = CMD_DISABLED;
        cmd.FVal[i] = CMD_DISABLED;
        cmd.jVal[i] = 0x800000; //Current position, 0x800000 is the centre
        cmd.IVal[i] = cmd.siderealIVal[i]; //Recieved Speed will be set by :I command.
        cmd.GVal[i] = CMD_GVAL_LOWSPEED_SLEW; //Mode recieved from :G command
        cmd.HVal[i] = 0; //Value recieved from :H command
        cmd.eVal[i] = _eVal; //version number
        cmd.gVal[i] = _gVal; //High speed scalar
        cmd.minSpeed[i] = cmd.accelTable[i][0].speed;//2x sidereal rate. [minspeed is the point at which acceleration curves are enabled]
        cmd.stopSpeed[i] = cmd.minSpeed[i];
        cmd.currentIVal[i] = cmd.stopSpeed[i]+1; //just slower than stop speed as axes are stopped.
        cmd.motorSpeed[i] = cmd.stopSpeed[i]+1; //same as above.
    }
    Commands_configureST4Speed(CMD_ST4_DEFAULT);
}

void Commands_configureST4Speed(byte mode) {
    cmd.st4Mode = mode;
    if (mode == CMD_ST4_HIGHSPEED) {
        //Set the ST4 speeds to highspeed standalone mode (goto speeds)
        cmd.st4RAIVal[ST4P] = cmd.normalGotoSpeed[RA];
        cmd.st4RAIVal[ST4N] = cmd.normalGotoSpeed[RA];
        cmd.st4RAReverse    = CMD_REVERSE;
        cmd.st4DecIVal      = cmd.normalGotoSpeed[DC];
    } else if (mode == CMD_ST4_STANDALONE) {
        //Set the ST4 speeds to standalone mode (2x around sidereal speed)
        cmd.st4RAIVal[ST4P] =(cmd.siderealIVal[RA])/3; //3x speed
        cmd.st4RAIVal[ST4N] =(cmd.siderealIVal[RA])  ; //-1x speed
        cmd.st4RAReverse    = CMD_REVERSE;
        cmd.st4DecIVal      =(cmd.siderealIVal[DC])/2; //2x speed
    } else {
        //Set the ST4 speeds to normal mode (0.25x around sidereal speed)
        cmd.st4RAIVal[ST4P] =(cmd.siderealIVal[RA] * 20)/(20 + cmd.st4SpeedFactor); //(1+SpeedFactor)x speed   -- Max. IVal = 1200, so this will never overflow.
        cmd.st4RAIVal[ST4N] =(cmd.siderealIVal[RA] * 20)/(20 - cmd.st4SpeedFactor); //(1-SpeedFactor)x speed
        cmd.st4RAReverse    = CMD_FORWARD;
        cmd.st4DecIVal      =(cmd.siderealIVal[DC] * 20)/( 0 + cmd.st4SpeedFactor); //(SpeedFactor)x speed
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
                                                 {'V', 2, 0},
                                                 //Programmer Entry Command
                                                 {'O', 1, 0},
                                                 //Programmer Commands - Ignored in Run-Mode
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
                                                 {'R', 6, 0},
                                                 {'r', 0, 6},
                                                 {'Q', 2, 0},
                                                 {'q', 0, 2},
                                                 {'o', 0, 2},
                                                 {'X', 6, 0},
                                                 {'x', 0, 6},
                                                 {'Y', 2, 0},
                                                 {'T', 1, 0}
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

