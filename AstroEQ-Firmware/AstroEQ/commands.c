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
        cmd.jVal[i] = CMD_DEFAULT_INDEX;
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

// Note: The protocol was changed after I added programming commands, so some of the
// commands used in programming mode are also used in run mode for different things.
// For most cases this is not an issue as there are checks in the command handling to
// see if we are in programming mode.
// In the case of the 'q' command, annoyingly I use it for a different payload and
// response length than the protocol does. In a dirty hack, the payload length is now
// specified as two nibbles, the lower being run-time, and the upper being prog mode.
 
#define CMD_LEN_SAME(b)    (((b) << 4) | ((b) & 0xF))
#define CMD_LEN_DIFF(r, p) (((p) << 4) | ((r) & 0xF))
#define CMD_GET_LEN(p, v) (p == CMD_LEN_RUN) ? ((v) & 0xF) :  ((v) >> 4)
 
const char cmd_commands[numberOfCommands][3] = { {'j', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  }, //arranged in order of most frequently used to reduce searching time.
                                                 {'f', CMD_LEN_SAME(0),   CMD_LEN_SAME(3)  },
                                                 {'I', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'G', CMD_LEN_SAME(2),   CMD_LEN_SAME(0)  },
                                                 {'J', CMD_LEN_SAME(0),   CMD_LEN_SAME(0)  },
                                                 {'K', CMD_LEN_SAME(0),   CMD_LEN_SAME(0)  },
                                                 {'H', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'M', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'e', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'a', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'b', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'g', CMD_LEN_SAME(0),   CMD_LEN_SAME(2)  },
                                                 {'s', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'E', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'P', CMD_LEN_SAME(1),   CMD_LEN_SAME(0)  },
                                                 {'F', CMD_LEN_SAME(0),   CMD_LEN_SAME(0)  },
                                                 {'L', CMD_LEN_SAME(0),   CMD_LEN_SAME(0)  },
                                                 {'V', CMD_LEN_SAME(2),   CMD_LEN_SAME(0)  },
                                                 {'q', CMD_LEN_DIFF(6,0), CMD_LEN_DIFF(6,2)},
                                                 //Programmer Entry Command
                                                 {'O', CMD_LEN_SAME(1),   CMD_LEN_SAME(0)  },
                                                 //Programmer Commands - Ignored in Run-Mode
                                                 {'A', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'B', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'S', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'n', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'N', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'D', CMD_LEN_SAME(2),   CMD_LEN_SAME(0)  },
                                                 {'d', CMD_LEN_SAME(0),   CMD_LEN_SAME(2)  },
                                                 {'C', CMD_LEN_SAME(1),   CMD_LEN_SAME(0)  },
                                                 {'c', CMD_LEN_SAME(0),   CMD_LEN_SAME(2)  },
                                                 {'Z', CMD_LEN_SAME(2),   CMD_LEN_SAME(0)  },
                                                 {'z', CMD_LEN_SAME(0),   CMD_LEN_SAME(2)  },
                                                 {'R', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'r', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'Q', CMD_LEN_SAME(2),   CMD_LEN_SAME(0)  },
                                                 {'o', CMD_LEN_SAME(0),   CMD_LEN_SAME(2)  },
                                                 {'X', CMD_LEN_SAME(6),   CMD_LEN_SAME(0)  },
                                                 {'x', CMD_LEN_SAME(0),   CMD_LEN_SAME(6)  },
                                                 {'Y', CMD_LEN_SAME(2),   CMD_LEN_SAME(0)  },
                                                 {'T', CMD_LEN_SAME(1),   CMD_LEN_SAME(0)  }
                                               };

char Commands_getLength(char cmd, bool sendRecieve, bool isProg){
    for(byte i = 0;i < numberOfCommands;i++){
        if(cmd_commands[i][0] == cmd){
            if(sendRecieve == CMD_LEN_SEND){
                return CMD_GET_LEN(isProg, cmd_commands[i][2]);
            } else {
                return CMD_GET_LEN(isProg, cmd_commands[i][1]);
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

