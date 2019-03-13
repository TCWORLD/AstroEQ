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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__
  
#include "AstroEQ.h"
#include "EEPROMReader.h" //Read config file

#define CMD_ST4_HIGHSPEED   2
#define CMD_ST4_STANDALONE  1
#define CMD_ST4_DEFAULT     0

#define CMD_FORWARD         false
#define CMD_REVERSE         true

#define CMD_STOPPED         true
#define CMD_RUNNING         false

#define CMD_ENABLED         true
#define CMD_DISABLED        false

#define CMD_GVAL_LOWSPEED_GOTO  2
#define CMD_GVAL_HIGHSPEED_GOTO 0
#define CMD_GVAL_LOWSPEED_SLEW  1
#define CMD_GVAL_HIGHSPEED_SLEW 3

typedef struct{        
    //class variables
    unsigned long    jVal           [2]; //_jVal: Current position
    unsigned int     IVal           [2]; //_IVal: speed to move if in slew mode
    unsigned int     motorSpeed     [2]; //speed at which moving. Accelerates to IVal.
    byte             GVal           [2]; //_GVal: slew/goto mode
    unsigned long    HVal           [2]; //_HVal: steps to move if in goto mode
    volatile char    stepDir        [2]; 
    bool             dir            [2];
    bool             FVal           [2];
    bool             gotoEn         [2];
    bool             stopped        [2];
    bool             highSpeedMode  [2];
    unsigned long    eVal           [2]; //_eVal: Version number
    unsigned long    aVal           [2]; //_aVal: Steps per axis revolution
    unsigned long    bVal           [2]; //_bVal: Sidereal Rate of axis
    byte             gVal           [2]; //_gVal: Speed scalar for highspeed slew
    unsigned long    sVal           [2]; //_sVal: Steps per worm gear revolution
    byte             st4Mode;            //Current ST-4 mode
    byte             st4SpeedFactor;     //Multiplication factor to get st4 speed. min = 1 = 0.05x, max = 19 = 0.95x.
    unsigned int     st4RAIVal      [2]; //_IVal: for RA ST4 movements ({RA+,RA-});
    bool             st4RAReverse;       //Reverse RA- axis direction if true.
    unsigned int     st4DecIVal;         //_IVal: for declination ST4 movements
    unsigned int     st4DecBacklash;     //Number of steps to perform on ST-4 direction change ---- Not yet implemented.
    unsigned int     siderealIVal   [2]; //_IVal: at sidereal rate
    unsigned int     currentIVal    [2]; //this will be updated to match the requested IVal once the motors are stopped.
    unsigned int     minSpeed       [2]; //slowest speed allowed
    unsigned int     normalGotoSpeed[2]; //IVal for normal goto movement.
    unsigned int     stopSpeed      [2]; //Speed at which mount should stop. May be lower than minSpeed if doing a very slow IVal.
    AccelTableStruct accelTable     [2][AccelTableLength]; //Acceleration profile now controlled via lookup table. The first element will be used for cmd.minSpeed[]. max repeat=85
} Commands;

#define numberOfCommands 39

void Commands_init(unsigned long _eVal, byte _gVal);
void Commands_configureST4Speed(byte mode);
char Commands_getLength(char cmd, bool sendRecieve);
  
//Command definitions
extern const char command[numberOfCommands][3];
extern Commands cmd;

//Methods for accessing command variables
inline void cmd_setDir(byte target, bool _dir){ //Set Method
    cmd.dir[target] = _dir ? CMD_REVERSE : CMD_FORWARD; //set direction
}

inline void cmd_updateStepDir(byte target, byte stepSize){
    if(cmd.dir[target] == CMD_REVERSE){
        cmd.stepDir[target] = -stepSize; //set step direction
    } else {
        cmd.stepDir[target] = stepSize; //set step direction
    }
}

inline unsigned int cmd_fVal(byte target){ //_fVal: 0hds000r000f; h=high speed, d = dir, s = slew, r = running, f = energised
    unsigned int fVal = 0;
    if (cmd.highSpeedMode[target]) {
        fVal |= (1 << 10);
    }
    if (cmd.dir[target]) {
        fVal |= (1 <<  9);
    }
    if (!cmd.gotoEn[target]) {
        fVal |= (1 <<  8);
    }
    if (!cmd.stopped[target]) {
        fVal |= (1 <<  4);
    }
    if (cmd.FVal[target]){
        fVal |= (1 <<  0);
    }
    return fVal;
}

inline void cmd_setsideIVal(byte target, unsigned int _sideIVal){ //set Method
    cmd.siderealIVal[target] = _sideIVal;
}

inline void cmd_setStopped(byte target, bool _stopped){ //Set Method
    cmd.stopped[target] = _stopped;
}

inline void cmd_setGotoEn(byte target, bool _gotoEn){ //Set Method
    cmd.gotoEn[target] = _gotoEn;
}

inline void cmd_setFVal(byte target, bool _FVal){ //Set Method
    cmd.FVal[target] = _FVal;
}

inline void cmd_setjVal(byte target, unsigned long _jVal){ //Set Method
    cmd.jVal[target] = _jVal;
}

inline void cmd_setIVal(byte target, unsigned int _IVal){ //Set Method
    cmd.IVal[target] = _IVal;
}

inline void cmd_setaVal(byte target, unsigned long _aVal){ //Set Method
    cmd.aVal[target] = _aVal;
}

inline void cmd_setbVal(byte target, unsigned long _bVal){ //Set Method
    cmd.bVal[target] = _bVal;
}

inline void cmd_setsVal(byte target, unsigned long _sVal){ //Set Method
    cmd.sVal[target] = _sVal;
}

inline void cmd_setHVal(byte target, unsigned long _HVal){ //Set Method
    cmd.HVal[target] = _HVal;
}

inline void cmd_setGVal(byte target, byte _GVal){ //Set Method
    cmd.GVal[target] = _GVal;
}

inline void cmd_setst4SpeedFactor(byte _factor){ //Set Method
    cmd.st4SpeedFactor = _factor;
}

inline void cmd_setst4DecBacklash(unsigned int _backlash){ //Set Method
    cmd.st4DecBacklash = _backlash;
}


#endif //__COMMANDS_H__
