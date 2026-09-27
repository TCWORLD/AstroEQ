
#ifndef __COMMANDS_H__
#define __COMMANDS_H__
  
#include "AstroEQ.h"
#include "EEPROMReader.h" //Read config file

typedef enum __attribute__((packed)){
    CMD_FORWARD,
    CMD_REVERSE
} MotorDir;

typedef enum __attribute__((packed)){
    CMD_RUNNING,
    CMD_STOPPED
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
    ST4TargetMode    st4Target;          //Current ST-4 target
    byte             st4SpeedFactor;     //Multiplication factor to get st4 speed. min = 1 = 0.05x, max = 19 = 0.95x.
    EmergencyStop    estop;
    unsigned int     st4RATrackIVal;     //_IVal: for RA ST4 tracking. Accounts for e.g. solar/lunar tracking modes
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
void Commands_configureST4Speed(ST4SpeedMode mode, ST4TargetMode target, MotorAxis axis, ST4EqmodSpeed speed);
char Commands_getLength(char cmd, CmdDirection sendRecieve, CmdProgMode isProg);
  
//Command definitions
extern const char command[numberOfCommands][3];
extern Commands cmd;

//Methods for accessing command variables
inline void cmd_setDir(MotorAxis axis, MotorDir dir){ //Set Method
    cmd.dir[axis] = dir; //set direction
}

inline void cmd_updateStepDir(MotorAxis axis, byte stepSize){
    if(cmd.dir[axis] == CMD_REVERSE){
        cmd.stepDir[axis] = -stepSize; //set step direction
    } else {
        cmd.stepDir[axis] = stepSize; //set step direction
    }
}

inline unsigned int cmd_fVal(MotorAxis axis){ //_fVal: 0hds00er000f; h=high speed, d = dir, s = slew, e = estop, r = running, f = energised
    unsigned int fVal = 0;
    if (cmd.highSpeedMode[axis] == CMD_HIGHSPEED) {
        fVal |= (1 << 10);
    }
    if (cmd.dir[axis] == CMD_REVERSE) {
        fVal |= (1 <<  9);
    }
    if (cmd.gotoEn[axis] == CMD_DISABLED) {
        fVal |= (1 <<  8);
    }
    if (cmd.estop == CMD_EMERGENCY) {
        fVal |= (1 <<  5);
    }
    if (cmd.stopped[axis] != CMD_STOPPED) {
        fVal |= (1 <<  4);
    }
    if (cmd.FVal[axis] == CMD_ENABLED){
        fVal |= (1 <<  0);
    }
    return fVal;
}

inline void cmd_setsideIVal(MotorAxis axis, unsigned int _sideIVal){ //set Method
    cmd.siderealIVal[axis] = _sideIVal;
}

inline void cmd_setStopped(MotorAxis axis, MotorRunning stopped){ //Set Method
    cmd.stopped[axis] = stopped;
}

inline void cmd_setEmergency(EmergencyStop estop){ //Set Method
    cmd.estop = estop;
}

inline void cmd_setGotoEn(MotorAxis axis, CmdEnabled gotoEn){ //Set Method
    cmd.gotoEn[axis] = gotoEn;
}

inline void cmd_setFVal(MotorAxis axis, CmdEnabled motor){ //Set Method
    cmd.FVal[axis] = motor;
}

inline void cmd_setjVal(MotorAxis axis, unsigned long _jVal){ //Set Method
    cmd.jVal[axis] = _jVal;
}

inline void cmd_setIVal(MotorAxis axis, unsigned int _IVal){ //Set Method
    cmd.IVal[axis] = _IVal;
}

inline void cmd_setaVal(MotorAxis axis, unsigned long _aVal){ //Set Method
    cmd.aVal[axis] = _aVal;
}

inline void cmd_setbVal(MotorAxis axis, unsigned long _bVal){ //Set Method
    cmd.bVal[axis] = _bVal;
}

inline void cmd_setsVal(MotorAxis axis, unsigned long _sVal){ //Set Method
    cmd.sVal[axis] = _sVal;
}

inline void cmd_setHVal(MotorAxis axis, unsigned long _HVal){ //Set Method
    cmd.HVal[axis] = _HVal;
}

inline void cmd_setGVal(MotorAxis axis, CmdSlewMode _GVal){ //Set Method
    cmd.GVal[axis] = _GVal;
}

inline void cmd_setST4SpeedFactor(byte _factor){ //Set Method
    cmd.st4SpeedFactor = _factor;
}

inline void cmd_setST4DecBacklash(unsigned int _backlash){ //Set Method
    cmd.st4DecBacklash = _backlash;
}


#endif //__COMMANDS_H__
