
#ifndef __COMMANDS_H__
#define __COMMANDS_H__
  
#include "AstroEQ.h"
#include "EEPROMReader.h" //Read config file

typedef enum __attribute__((packed)){
    CMD_FORWARD,
    CMD_REVERSE
} MotorDir;

typedef enum __attribute__((packed)){
    CMD_STOPPED,
    CMD_RUNNING
} MotorRunning;

typedef enum  __attribute__((packed)){
    CMD_LOWSPEED,
    CMD_HIGHSPEED
} MotorSpeed;

typedef enum  __attribute__((packed)){
    CMD_DISABLED, 
    CMD_ENABLED
} CmdEnabled;

typedef enum __attribute__((packed)){
    CMD_NORMAL,
    CMD_EMERGENCY
} EmergencyStop;

#define CMD_DEFAULT_INDEX 0x800000 //Current position, 0x800000 is the centre

typedef enum __attribute__((packed)){
    CMD_GVAL_HIGHSPEED_GOTO = 0,
    CMD_GVAL_LOWSPEED_SLEW,
    CMD_GVAL_LOWSPEED_GOTO,
    CMD_GVAL_HIGHSPEED_SLEW
} CmdSlewMode;

typedef enum __attribute__((packed)){
    CMD_LEN_SEND,
    CMD_LEN_RECV
} CmdDirection;

typedef enum __attribute__((packed)){
    CMD_LEN_RUN,
    CMD_LEN_PROG
} CmdProgMode;

typedef struct{        
    //class variables
    unsigned long    jVal           [2]; //_jVal: Current position
    unsigned int     IVal           [2]; //_IVal: speed to move if in slew mode
    unsigned int     motorSpeed     [2]; //speed at which moving. Accelerates to IVal.
    unsigned long    HVal           [2]; //_HVal: steps to move if in goto mode
    CmdSlewMode      GVal           [2]; //_GVal: slew/goto mode
    int8_t           stepDir        [2]; 
    MotorDir         dir            [2];
    CmdEnabled       FVal           [2];
    CmdEnabled       gotoEn         [2];
    MotorRunning     stopped        [2];
    MotorSpeed       highSpeedMode  [2];
    byte             gVal           [2]; //_gVal: Speed scalar for highspeed slew
    unsigned long    eVal           [2]; //_eVal: Version number
    unsigned long    aVal           [2]; //_aVal: Steps per axis revolution
    unsigned long    bVal           [2]; //_bVal: Sidereal Rate of axis
    unsigned long    sVal           [2]; //_sVal: Steps per worm gear revolution
    MotorDir         st4RAReverse;       //Reverse RA- axis direction if true.
    ST4SpeedMode     st4Mode;            //Current ST-4 mode
    byte             st4SpeedFactor;     //Multiplication factor to get st4 speed. min = 1 = 0.05x, max = 19 = 0.95x.
    EmergencyStop    estop;
    unsigned int     st4RAIVal      [2]; //_IVal: for RA ST4 movements ({RA+,RA-});
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
void Commands_configureST4Speed(ST4SpeedMode mode, MotorAxis target, ST4EqmodSpeed speed);
char Commands_getLength(char cmd, CmdDirection sendRecieve, CmdProgMode isProg);
  
//Command definitions
extern const char command[numberOfCommands][3];
extern Commands cmd;

//Methods for accessing command variables
inline void cmd_setDir(MotorAxis target, MotorDir dir){ //Set Method
    cmd.dir[target] = dir; //set direction
}

inline void cmd_updateStepDir(MotorAxis target, byte stepSize){
    if(cmd.dir[target] == CMD_REVERSE){
        cmd.stepDir[target] = -stepSize; //set step direction
    } else {
        cmd.stepDir[target] = stepSize; //set step direction
    }
}

inline unsigned int cmd_fVal(MotorAxis target){ //_fVal: 0hds00er000f; h=high speed, d = dir, s = slew, e = estop, r = running, f = energised
    unsigned int fVal = 0;
    if (cmd.highSpeedMode[target] == CMD_HIGHSPEED) {
        fVal |= (1 << 10);
    }
    if (cmd.dir[target] == CMD_REVERSE) {
        fVal |= (1 <<  9);
    }
    if (cmd.gotoEn[target] == CMD_DISABLED) {
        fVal |= (1 <<  8);
    }
    if (cmd.estop == CMD_EMERGENCY) {
        fVal |= (1 <<  5);
    }
    if (cmd.stopped[target] == CMD_STOPPED) {
        fVal |= (1 <<  4);
    }
    if (cmd.FVal[target] == CMD_ENABLED){
        fVal |= (1 <<  0);
    }
    return fVal;
}

inline void cmd_setsideIVal(MotorAxis target, unsigned int _sideIVal){ //set Method
    cmd.siderealIVal[target] = _sideIVal;
}

inline void cmd_setStopped(MotorAxis target, MotorRunning stopped){ //Set Method
    cmd.stopped[target] = stopped;
}

inline void cmd_setEmergency(EmergencyStop estop){ //Set Method
    cmd.estop = estop;
}

inline void cmd_setGotoEn(MotorAxis target, CmdEnabled gotoEn){ //Set Method
    cmd.gotoEn[target] = gotoEn;
}

inline void cmd_setFVal(MotorAxis target, CmdEnabled motor){ //Set Method
    cmd.FVal[target] = motor;
}

inline void cmd_setjVal(MotorAxis target, unsigned long _jVal){ //Set Method
    cmd.jVal[target] = _jVal;
}

inline void cmd_setIVal(MotorAxis target, unsigned int _IVal){ //Set Method
    cmd.IVal[target] = _IVal;
}

inline void cmd_setaVal(MotorAxis target, unsigned long _aVal){ //Set Method
    cmd.aVal[target] = _aVal;
}

inline void cmd_setbVal(MotorAxis target, unsigned long _bVal){ //Set Method
    cmd.bVal[target] = _bVal;
}

inline void cmd_setsVal(MotorAxis target, unsigned long _sVal){ //Set Method
    cmd.sVal[target] = _sVal;
}

inline void cmd_setHVal(MotorAxis target, unsigned long _HVal){ //Set Method
    cmd.HVal[target] = _HVal;
}

inline void cmd_setGVal(MotorAxis target, CmdSlewMode _GVal){ //Set Method
    cmd.GVal[target] = _GVal;
}

inline void cmd_setST4SpeedFactor(byte _factor){ //Set Method
    cmd.st4SpeedFactor = _factor;
}

inline void cmd_setST4DecBacklash(unsigned int _backlash){ //Set Method
    cmd.st4DecBacklash = _backlash;
}


#endif //__COMMANDS_H__
