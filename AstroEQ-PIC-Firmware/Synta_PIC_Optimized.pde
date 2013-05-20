/*
  Code ported by Joël Collet 2013 for Pinguino adaptation

  Thanks to  Thomas Carpenter 2013 for the original code
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Syntia
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts (Not EQ3-2, they have a different gear ratio)
 
  Current Version: 0.1
*/

//#include "synta_cmd.h" //Synta Communications Protocol.

//The system needs to be configured to suit your mount.
//Either use the instructions below and calculate the values yourself,
//or use the "Initialisation Variable Calculator.xlsx" file.
//
// The line to configure the mount follows these instructions!
//
//
//  Format:
//
//Synta Synta( e [1281],  a,  b,  g [16],  s);
//
// e = version number (this doesnt need chaning)
//
// a = number of steps per complete axis revolution. 
// aVal = µsteps/steps * motor steps per revolution where µsteps/steps maximum value is 32 for drv8825 and 16 for 4988 boards
// where ratio is the gear ratio from motor to axis. E.g. for an HEQ5 mount, this is 705
// and motor steps is how many steps per one revolution of the motor. E.g. a 1.8degree stepper has 200 steps/revolution
//
// b = sidereal rate. This again depends on number of microsteps per step. 
// bVal = 620 * aVal / 86164.0905   (round UP to nearest integer)
//
// g = high speed multiplyer. This is the EQMOD recommended default. In highspeed mode, the step rate will be 16 times larger.
//actually the g value is closely related to the stepping mode choosed in high speed mode
//that is if you are  to go in high speed mode from say 1/64 µstep to 1/8 µtep, the g value is 8, not 16!!
// otherwise your speed is 2 times slower than wanted
// s = steps per worm revolution
// sVal = aVal / Worm Gear Teeth
// where worm gear teeth is how many teeth on the main worm gear. E.g. for an HEQ5 this is 135
//
//
// Examples:
//
// mount specifics can be found here: http://tech.groups.yahoo.com/group/EQMOD/database?method=reportRows&tbl=5
//
// Summary of mounts (Untested):
//
// HEQ5 Pro/Syntrek/SynscanUpgrade:
// aVal = 9024000
// bVal = 64933
// sVal = 66845
// scalar = 1
//
// EQ6 Pro/Syntrek/SynscanUpgrade:
// aVal = 9024000
// bVal = 64933
// sVal = 50133
// scalar = 1
//
// EQ5 Synscan/SynscanUpgrade:
// aVal = 9011200
// bVal = 64841
// sVal = 31289
// scalar = 1
//
// 
// Other mounts:
// aVal = µsteps/step * motor steps per revolution * ratio (where µsteps/steps depends of the driver board and mode used
// bVal = 620 * aVal / 86164.0905   (round to nearest whole number)
// sVal = aVal / Worm Gear Teeth   (round to nearest whole number)
// 
//
//Create an instance of the mount
//If aVal is roughly greater than 10000000 (ten million), the number needs to be scaled down by some factor
//to make it smaller than that limit. In my case I have chosen 10 as it nicely scales the
//a and s values. Hence below, I have set scalar to 10.

//Define the two axes (swap the 0 and 1 if R.A. and Dec. motors are reversed)
//-------------------------------------------------------------
//DO NOT EDIT ANYTHING BELOW THIS LINE-------------------------
//-------------------------------------------------------------

#include <pic18fregs.h>
#include <typedef.h>
#include <macro.h>
#include <math.h>

#define INT_ENABLE  		1
#define INT_DISABLE			0
#define INT_ENABLE_PRIORITY	1
#define INT_DISABLE_PRIORITY 0
#define INT_HIGH_PRIORITY	1
#define INT_LOW_PRIORITY	0

#define T3_PS_1_8			0b11111111 // 1:8 Prescale value
#define T3_PS_1_1			0b11001111 // 1:1 Prescale value

#define T1_PS_1_1			0b11001111	// 1:1 prescale value
#define T1_PS_1_8			0b11111111	// 1:8 prescale value


#ifndef USERINT  //pour activer les interruptions
#define USERINT
#endif
  
#ifndef INT0INT //pour activer les interruptions high
#define INT0INT
#endif


#define NORMALGGOTOSPEED 320L //This is defined as Speed with the letter 'L' appended
#define FASTGOTOSPEED 176L
//#define USEHIGHSPEEDMODE 1     
//#define HAVEST4 1 //in case we want to have ST4 input
//#define HAVEENCODER 1 //if we want to implement encoder (as in the GT mount)
// We can not have both ST4 and ENCODER..not enough interruptible pins
      

//variable definition 

unsigned long gotoPosn[2] = {0,0}; //where to slew to
unsigned int timerOVF[2]; //Clock section of each of the 64 microsteps
unsigned long timerCountRate;
unsigned long maxTimerOVF; //Minimum allowable clock count
u8 rate_modifier = 2; //2 =(0,25); 1=(0,5), 0 =(1x) for  guiding 0.75x is not proposed so far, due to more complex handling
#define PULSEWIDTH 5 //4µs, dans delayus

//Pins
//const char statusPin = 13; //RA4, USERLED
#ifdef HAVEST4
const char ST4Pin[4] = {4,5,6,7}; // RB4 to RB7 ST4 guiding input
#endif
#ifdef HAVEENCODER
const char ENCODERPin[4] = {4,5,6,7}; // RB4 to RB7 RA and DEC encoders input
#endif

const char resetPin[2] = {12,17}; //RC2,RA5

const char dirPin[2] = {0,1}; //RB0,RB1
const char enablePin[2] = {2,3}; //RB2,RB3
const char stepPin[2] = {10,11}; //RC0 et RC1 //RA DEC
const char modePins[2][2] = {{13,14},{15,16}}; //RA0,RA1,RA2,RA3 {RA{Mode1,Mode2}, DEC{Mode1,Mode2}} Modepins.
//see datasheet for pololu card, mode0 = ms1
//drv8825 32 µpas, mode0,1,2 (0,1,0) =1/4 steps (0,1,1)=1/32 steps  =>mode0 always connected to 0 (which allows also  1/16 with (0,0,1)
//a4988, 16 µpas, ms123(1,0,0)=1/2 steps (1,1,1)=1/16steps => ms1 => always 1
//if g value is 16 then we want
//drv8825 in 1/2 step mode mode0,1,2, (1,0,0) and (1,1,1) for 1/32 steps mode 1=mode2
//a4988 in fullstep mode ms123=(0,0,0), need to connect differently, that is ms1=ms2=ms3!
//maybe one mode pin command can be used for that purpose, and we gain 2 more pins, for example for emergency stop/limit detection
#ifdef USEHIGHSPEEDMODE
boolean highSpeed[2] = {false,false}; //whether number of microsteps is dropped by 8 (16?) due to high speed (to increase Torque) -
                                      //If you want to use it, uncomment the line: '#define USEHIGHSPEEDMODE 1'
                                      //and connect the mode pins of the DRV8824PWP chips to the pins shown
#endif

//

//synta command part
#define numberOfCommands 17

#include "string.h"
//#include "stdlib.h"
#include "stdio.h"

const char startInChar = ':';
const char startOutChar = '=';
const char errorChar = '!';
const char endChar = '\r';
const char command[numberOfCommands][3] = { {'e', 0, 6},
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

unsigned long _jVal[2]; //_jVal: Current position
unsigned long _IVal[2]; //_IVal: speed to move if in slew mode
byte _GVal[2]; //_GVal: slew/goto mode
unsigned long _HVal[2]; //_HVal: steps to move if in goto mode
//unsigned int _flag[2]; //_fVal: 00ds000g000f; d = dir, s = stopped, g = goto, f = energised
char stepDir[2];
byte dir[2];
byte FVal[2];
byte gotoEn[2];
byte stopped[2];
unsigned long _eVal[2]; //_eVal: Version number
unsigned long _aVal[2]; //_aVal: Steps per axis revolution
unsigned long _bVal[2]; //_bVal: Sidereal Rate of axis
byte _gVal[2]; //_gVal: Speed scalar for highspeed slew
unsigned long _sVal[2]; //_sVal: Steps per worm gear revolution

boolean validPacket;
char commandString[11];
char commandIndex;
      
byte _axis;
char _command;
//byte _scalar; 
//byte div_scalar;

//Command part
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

void Synta_clearBuffer(char* buf, byte len){
  strncpy(buf,"",len);
}

char Synta_cmd_getLength(char cmd, boolean sendRecieve){
byte i;
  for(i = 0;i < numberOfCommands;i++){
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
 
unsigned int Synta_cmd_fVal(byte target){ //calculate the status flag to be returned to eqmod
  return ((dir[target] << 9)|(stopped[target] << 8)|(gotoEn[target] << 4)|(FVal[target] << 0));
}
 
 
void Synta_init(unsigned long eVal,unsigned long aVal,unsigned long bVal,byte gVal,unsigned long sVal){
    byte i;
    validPacket = 0;
  _axis = 0;
  commandIndex = 0;
  Synta_clearBuffer(commandString,sizeof(commandString));
  //_scalar = scalar;*
  for( i = 0;i < 2;i++){
    dir[i] = 0;
    stopped[i] = 1;
    gotoEn[i] = 0;
    FVal[i] = 0;
    _jVal[i] = 0x800000; //Current position, 0x800000 is the centre
    _IVal[i] = 0; //Recieved Speed
    _GVal[i] = 1; //Mode recieved from :G command
    _HVal[i] = 0; //Value recieved from :H command
    _eVal[i] = eVal; //version number
    _aVal[i] = aVal; //steps/axis
    _bVal[i] = bVal; //sidereal rate
    _gVal[i] = gVal; //High speed scalar
    _sVal[i] = sVal; //steps/worm rotation
  }

}


void Synta_error(char* buf){
  buf[0] = errorChar;
  buf[1] = endChar;
  buf[2] = 0;
}

boolean Synta_validateCommand(byte len){
char requiredLength;
byte i;
  _command = commandString[0]; //first byte is command
  _axis = commandString[1] - 49; //second byte is axis
  if(_axis > 1){
    return 0; //incorrect axis
  }
  requiredLength = Synta_cmd_getLength(_command,1); //get the required length of this command
  len -= 3; //Remove the command and axis bytes, aswell as the end char;
  if(requiredLength != len){ //If invalid command, or not required length
    return 0;
  }
  
  
  for(i = 0;i < len;i++){
    commandString[i] = commandString[i + 2];
  }
  commandString[i] = 0; //Null
  return 1;
}

char Synta_recieveCommand(char* dataPacket, char character){
  if(validPacket){
        if (character == startInChar){
      goto error; //new command without old finishing! (dataPacket contains error message)
    }
    
    commandString[commandIndex++] = character; //Add character to current string build
 
    if(character == endChar){
      if(Synta_validateCommand(commandIndex)){
        strcpy(dataPacket,commandString); //Return decoded packet
        validPacket = 0;
        return 1; //Successful decode (dataPacket contains decoded packet)
      } else {
        goto error; //Decode Failed (dataPacket contains error message)
      }
    } else if (commandIndex == sizeof(commandString)){
      goto error; //Message too long! (dataPacket contains error message)
    }
  } else if (character == startInChar){
    //Begin new command

    commandIndex = 0;
    validPacket = 1;
    Synta_clearBuffer(commandString,sizeof(commandString));
  }
 
  return 0; //Decode not finished (dataPacket unchanged)
error:
  Synta_error(dataPacket);
  validPacket = 0;
  return -1;
}

 
unsigned long Synta_hexToLong(char* hex){


unsigned long valeur = 0;
unsigned long val=0;
int i;
for (i=0;i<3;i++)
  {
       valeur=(int)hex[i*2+1];
       if (valeur <58) { val += (valeur-48)<<8*i;} else {val += (valeur-55)<<8*i;}
       valeur=(int)hex[i*2];
       if (valeur <58) { val += (valeur-48)<<(8*i+4);} else {val += (valeur-55)<<(8*i+4);}
  }
 
  return val;
}

void long_tohex(unsigned long dita){
  byte bytes[3];
  bytes[0] = (dita >> 16) & 0xFF;
  bytes[1] = (dita >> 8) & 0xFF;
  bytes[2] = (dita) & 0xFF;
  Serial.printf("=%02X%02X%02X\r",bytes[2],bytes[1],bytes[0]);
}
void byte_tohex( byte dita){
  dita &= 0xFF;
  Serial.printf("=%02X\r",dita);
}
void int_tohex( int dita){
  dita &= 0xFFF;
  Serial.printf("=%03X\r",dita);
}
  
 
// end of synta command part
    
// hardware part  
void speedCurve(unsigned long currentSpeed, char dir, byte steps, byte motor){ //dir = -1 or +1. - means accellerate
    unsigned long speedPerStep = (maxTimerOVF - timerOVF[motor]); 
    unsigned int microDelay;
    int i;
                //maxTimerOVF is always largest
     speedPerStep /= steps;
    speedPerStep >>= 1;
                  // Serial.printf("in speed curve %i\r", speedPerStep); //debug

    if (speedPerStep == 0){
        speedPerStep = 1; //just in case - prevents rounding from resulting in delay of 0.
    }
    microDelay = currentSpeed >> 1; //number of uS to wait for speed (rate is in 0,666 of a microsecond)
                 //  Serial.printf("in speed curve delay %i\r", microDelay);//debug

    for(i = 0;i < steps;i++){
        delayMicroseconds(microDelay);
        if(motor){ //StepStep DEC
                          // Serial.printf("in speed curve DE \r");//debug

            PORTCbits.RC1=1;
            delayMicroseconds(5);
            PORTCbits.RC1=0;
        } else {//Step RA

                                     //  Serial.printf("in speed curve RA \r");//debug
            PORTCbits.RC0=1;
            delayMicroseconds(5);
            PORTCbits.RC0=0;
        }
        microDelay += (dir * speedPerStep);
    }
}

void decellerate(byte decellerateSteps, byte motor){
    #if defined(USEHIGHSPEEDMODE)
    if(highSpeed[motor]){
        //Disable Highspeed mode on board
        highSpeed[motor] = false;
        digitalWrite(modePins[motor][0],HIGH); //(1/32 steps for drv8825 et 1/16 for a4988)
        digitalWrite(modePins[motor][1],HIGH);
   if(motor){ //DEC back to normal rythm, count 8 times less  than in high speed

                                                //  Serial.printf("in speed decel DE \r");//debug
        //PIE2bits.CCP2IE = INT_DISABLE;
        CCPR2H =  high8(timerOVF[motor]);//set compare value, need to check if we should disable the interrupt before setting register
        CCPR2L =  low8(timerOVF[motor]);//then re-enable it. if needed, uncomment and also add everywhere wher we set CCPR
        //PIE2bits.CCP2IE = INT_ENABLE; //the time for this set/unset is anyway very short (few 10 µs max), so not important
    } else {//RA
                                        //  Serial.printf("in speed decel RA \r");//debug
 
        //PIE1bits.CCP1IE = INT_DISABLE;
        CCPR1H =  high8(timerOVF[motor]);//set compare value 
        CCPR1L =  low8(timerOVF[motor]);
        //PIE1bits.CCP1IE = INT_DISABLE;
   }
 
    }
    #endif
      //x steps to slow down to maxTimerOVF
 //                                Serial.printf("in decell \r");//debug

    speedCurve(timerOVF[motor], 1, decellerateSteps, motor);
      //Now at minimum speed, so safe to stop.
}

void accellerate(byte accellerateSteps, byte motor){
    //x steps to speed up to timerOVF[Synta.axis()]
                               //Serial.printf("in accel \r");//Debug

    speedCurve(maxTimerOVF, -1, accellerateSteps, motor);
    //Now at speed, so check highspeed mode and then return. Timers will be used from now on.
    #if defined(USEHIGHSPEEDMODE)
    if(highSpeed[motor]){//Enable Highspeed mode on board
        digitalWrite(modePins[motor][0],LOW); //A4988
        digitalWrite(modePins[motor][1],LOW); 
        // digitalWrite(modePins[motor][0],HIGH); //drv8825
        // digitalWrite(modePins[motor][1],LOW); 
    if(motor){ //DEC 8 times less µsteps  (from  1/16 to  1/2 or 1/32 to 1/4, more torque available, count 8times more
        CCPR2H =  high8(_gVal[motor]*timerOVF[motor]);//set compare value,8 times longer we can do it because in highspeed mode the timer value
        CCPR2L =  low8(_gVal[motor]*timerOVF[motor]); //is always less than 65535/8 =8192
                               //Serial.printf("in accel DEC\r");//Debug


        } else {//RA
        CCPR1H =  high8(_gVal[motor]*timerOVF[motor]);//set compare value 8 times longer
        CCPR1L =  low8(_gVal[motor]*timerOVF[motor]);
                              //Serial.printf("in accel RA\r");//Debug

        }
    }
    #endif
}



 
void motorStopp(byte motor, byte steps){
    if (!stopped[motor]){
        if(motor){   //CCP2=DEC
            PIE2bits.CCP2IE = INT_DISABLE;
        } else {
            PIE1bits.CCP1IE = INT_DISABLE;
        }
        //Only decellerate if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
        if (gotoEn[motor]){
            gotoEn[motor]=0; //Cancel goto mode
        }//
        decellerate(steps,motor);
        _jVal[motor]=_jVal[motor] + stepDir[motor]*steps;
        stopped[motor]=1; //mark as stopped*/
    }
}
   
     
void motorStop(byte motor){
    byte numstep;
    numstep =60;
          //     Serial.printf("stop\r");//debug
 
    motorStopp(motor, numstep); //Default number of steps. Replaces function prototype from Version < 5.0
}      

void checkIfDone(byte motor){

 //il faut introduire une variable de type multiplier (decallage de type rolling) a la place de gval
 //cette variable est mise a 0 si on est pas e high speed dans la fonction accelerate et remise à 0dans la focntion decellerate si elle à une autre valeur
 //sinon elle est mise à gval binaire (si gval =2, multilpier =1, si gval =4 multilpier =2, si 8 multiliper=3 si 16 multiliper=4
 //cela permet d'eviter le test et de reintroduire la mise a jour du biniou dans l'inetrruption
 //le test gotoEn est aussi das l'interrupt, mais à la fin avec une variable motor settee dans le traitement de l'interruption
 //justea apres la levee du flag (si on est en fin de goto, on appelle motor stop (qui disable l'interrupt)
 //masi l'inetrrupt n'est pas cleare c'est pas bon!
 //ou alors on cleare dans motorstop, auc hoix a verifier ce qui est mieux! 
 //maybe it can be made more efficient to perform these operation in the inetrruption, but more effeciently, e.g using 
 // shift function instead of multiply. also the test can be removed, since we can set the shift value to 0. 
 //Shifting value will e setlled in the accelerate code and cleared in the deceleratte one
 //only the gotoEn is to be tested. but it can be placed also in theinterrupt, once motors are stepped
 // (ideally we can set a byte flag telin which motor was stepped, to avoid doubling the codefor this gotoen test
 //take care also to clear the interrupt flag before going into the motorstop code (that's cleaner i think)
  
   
    byte num;
    #if defined(USEHIGHSPEEDMODE)
    if(highSpeed[motor]){
        _jVal[motor]=_jVal[motor] + stepDir[motor]*_gVal[motor];

     } else {
    #else
        if(1){ //Always do the next line. Here to account for the extra close brace
        //} -> this one is supposed to be commented out! (keeps the IDE happy)
    #endif
        _jVal[motor]=_jVal[motor] + stepDir[motor];
   }
    if(gotoEn[motor]){
        long stepsLeft = gotoPosn[motor] - _jVal[motor];
        stepsLeft *= stepDir[motor];
         num =120;
        if(stepsLeft <= num){
            motorStopp(motor, stepsLeft); //will decellerate the rest of the way.
        }
    }
}       

 //motorStep is removed since scalar is removed and unused

/*void motorStep(byte motor){
static char divider[2] = {0};
divider[motor] += stepDir[motor];
if(stepDir[motor] < 0){
    if (divider[motor] < 0){
        divider[motor] = _scalar - 1;
        checkIfDone(motor);
    }
    } else {
        if (divider[motor] >= _scalar){
            divider[motor] = 0;
            checkIfDone(motor);
        }
    }
}*/

void motorEnable(){
//    char s;
  //  s = _axis;
    digitalWrite(enablePin[_axis],LOW);
    FVal[_axis]=1;
}



void motorStart(byte motor, byte steps){
    digitalWrite(dirPin[motor],dir[motor]); //set the direction
    stopped[motor]= 0;
    accellerate(steps,motor); //accellerate to calculated rate in given number of steps steps
    _jVal[motor]=_jVal[motor] + stepDir[motor]*steps;
    if(motor){ //DEC
        CCPR2H =  high8(timerOVF[motor]);//set compare value
        CCPR2L =  low8(timerOVF[motor]);
        PIE2bits.CCP2IE = INT_ENABLE; //enable interrupt (stepping is started)
        //T3CONbits.TMR3ON = ON; //don't know if we start counter only when motors run or if we let counter running all the time
    } else {//RA
        CCPR1H =  high8(timerOVF[motor]);//set compare value
        CCPR1L =  low8(timerOVF[motor]);
        PIE1bits.CCP1IE = INT_ENABLE;
        //T1CONbits.TMR1ON = ON;//
    }
}

void calculateRate(byte motor){
//seconds per step = IVal / BVal; for lowspeed move
//or seconds per step = IVal / (BVal * gVal); for highspeed move 
//
//whether to use highspeed move or not is determined by the GVal;
//
//clocks per step = timerCountRate * seconds per step
    unsigned long rate;
    float float_rate;
    unsigned long divisor = _bVal[motor];
    if((_GVal[motor] == 2)||(_GVal[motor] == 1)){ //Normal Speed
    #if defined(USEHIGHSPEEDMODE)
        //check if at high torque/high speed
        if (_IVal[motor] < 30){
            highSpeed[motor] = true;
        } else {
            highSpeed[motor] = false;
        }
    #endif
    } else if ((_GVal[motor] == 0)||(_GVal[motor] == 3)){ //High Speed
        divisor *= _gVal[motor];
    #if defined(USEHIGHSPEEDMODE)
        //check if at very high speed/high torque
        if (_IVal[motor] < (30 * _gVal[motor])){
            highSpeed[motor] = true;
        } else {
            highSpeed[motor] = false;
        }   
    #endif
    }
    rate = timerCountRate * _IVal[motor];
 //       Serial.printf("calcul rate %li\r",rate);//debug

    float_rate = (float)rate / divisor; //Calculate the quotient
 //       Serial.printf("calcul rate %f\r",float_rate); //Debug

    //Then convert the remainder into a decimal number (division of a small number by a larger one, improving accuracy)
    float_rate *= 10.0; //corrects for timer count rate being set to 1/10th of the required to prevent numbers getting too big for an unsigned long.
 
 //           Serial.printf("calcul rate %f\r",float_rate);//Debug
   //If there is now any integer part of the remainder after the multiplication by 10, it needs to be added back on to the rate and removed from the remainder.
   //rate = (unsigned long)floorf(float_rate);
           //  Serial.printf("calcul rate %li\r",rate); // debug

rate =(unsigned long)1*float_rate;
            //Serial.printf("calcul rate %li\r",rate); // debug

    if ((float_rate - (float)rate)>0.5 ) { //rounding to the nearest integer
        rate +=1; // is there a better solution?
    }
  //  Serial.printf("calcul rate %li\r",rate);//Debug
//Now truncate to an unsigned int with a sensible max value (the int is to avoid register issues with the 16 bit timer)
    if(rate > 64997UL){
        rate = 64997UL;
    } else if (rate < 20UL) { //to allow for goto higher than 400 ...but without the highspeed option itis hard to achieve
        rate = 20L;
    }
    timerOVF[motor] = rate;
          Serial.printf("calcul rate %li\r",rate); // 

  //  Serial.printf("calcul rate %li\r",rate);//Debug
}

void slewMode(){
    byte numstep;
//    numstep =Synta.scalar();
    //numstep =30/_scalar;
    numstep = 30;
    motorStart(_axis, numstep); //Begin PWM
}

void gotoMode(){
    byte num;
//    num =Synta.scalar();
   // num =140/_scalar;
   num = 140;
    if (_HVal[_axis] < num){
        if (_HVal[_axis] < 2){
            _HVal[_axis]=2;
    }
    //To short of a goto to use the timers, so do it manually
       // accellerate( (_HVal[_axis] / 2) * _scalar, _axis);
       // decellerate( (_HVal[_axis] / 2) * _scalar, _axis);
       accellerate( (_HVal[_axis] / 2), _axis);
       decellerate( (_HVal[_axis] / 2), _axis);
       
        _jVal[_axis]=_jVal[_axis] + stepDir[_axis] * _HVal[_axis]  ;
    } else {
        gotoPosn[_axis] = _jVal[_axis] + stepDir[_axis] * _HVal[_axis]; //current position + distance to travel
 //       num =Synta.scalar();
        //num =200/_scalar;
        num = 200;
        if (_HVal[_axis] < num){
            _IVal[_axis]= NORMALGGOTOSPEED;
            calculateRate(_axis);
     //       num =Synta.scalar();
     //num =20/_scalar;
            num =20;
            motorStart(_axis,num); //Begin PWM
        } else {
            _IVal[_axis]= FASTGOTOSPEED; //Higher speed goto
            calculateRate(_axis);
            //num =Synta.scalar();
            //num =60/_scalar
            num =60;
            motorStart(_axis,num); //Begin PWM
        }
        gotoEn[_axis]=1;
    }
}

//i know the following is not so nice compared to the original solution, but the original solution do not work under pinguino
// there is issues with the data pointers, which prevent to use the assemble_response function, so have done this way
// also, this is smaller in code size!!


void decodeCommand(char command, char* packetIn){ //each command is axis specific. The axis being modified can be retrieved by calling Synta.axis()
    unsigned long responseData; //data for response
    switch(command){
        case 'e': //readonly, return the eVal (version number)
            responseData = _eVal[_axis]; //response to the e command is stored in the eVal function for that axis.
            long_tohex(responseData);
        break;
        case 'a': //readonly, return the aVal (steps per axis)
            //responseData = _aVal[_axis]; //response to the a command is stored in the aVal function for that axis.
            //responseData /= _scalar;
            responseData = _aVal[_axis];
            long_tohex(responseData);
        break;
        case 'b': //readonly, return the bVal (sidereal rate)
            responseData = _bVal[_axis]; //response to the b command is stored in the bVal function for that axis.
            //responseData = _bVal[_axis];
            //responseData /= _scalar;
            //if (_bVal[_axis] % _scalar){
            //    responseData++; //round up
            //}
            long_tohex(responseData);
        break;
        case 'g': //readonly, return the gVal (high speed multiplier)
            responseData = _gVal[_axis]; //response to the g command is stored in the gVal function for that axis.
            byte_tohex(responseData);
        break;
        case 's': //readonly, return the sVal (steps per worm rotation)
            responseData = _sVal[_axis]; //response to the s command is stored in the sVal function for that axis.
            //responseData = _sVal[_axis];
            //responseData /= _scalar;
            long_tohex(responseData);
        break;
        case 'f': //readonly, return the fVal (axis status)
            responseData = Synta_cmd_fVal(_axis); //response to the f command is stored in the fVal function for that axis.
            int_tohex(responseData);  
        break;
        case 'j': //readonly, return the jVal (current position)
            responseData = _jVal[_axis]; //response to the j command is stored in the jVal function for that axis.
            long_tohex(responseData);
        break;
        case 'P':
            if ((packetIn[0] - 48) == 0) {
                rate_modifier = 0; 
            } else if ((packetIn[0] - 48) == 2) {
                rate_modifier = 1; 
            } else if ((packetIn[0] - 48) == 3) {
                rate_modifier = 2; 
            }
            Serial.printf("=\r");
        break;
      
        case 'K': //stop the motor, return empty response
            motorStop(_axis); //No specific response, just stop the motor (Feel free to customise motorStop function to suit your needs
            //responseData = 0;
            Serial.printf("=\r");
        break;
        case 'L': //emergency stop, return empty response
            motorStopp(0,1); //No specific response, just stop the motor (Feel free to customise motorStop function to suit your needs
            motorStopp(1,1); //No specific response, just stop the motor (Feel free to customise motorStop function to suit your needs
            //responseData = 0;
            Serial.printf("=\r");
        break;
        case 'G': //set mode and direction, return empty response
            _GVal[_axis]= (packetIn[0] - 48); //Store the current mode for the axis
            dir[_axis]= (packetIn[1] - 48); //Store the current direction for that axis
            if (dir[_axis] == 0) stepDir[_axis]=1; 
            else stepDir[_axis]=-1;

            //responseData = 0;
            Serial.printf("=\r");
           // Serial.printf("Gval %i\r",_GVal[_axis]);//debug
        break;
        case 'H': //set goto position, return empty response (this sets the number of steps to move from cuurent position if in goto mode)
            _HVal[_axis] = Synta_hexToLong(packetIn); //set the goto position container (convert string to long first)
            //responseData = 0;
            Serial.printf("=\r");
           //             Serial.printf("Hval %i\r",_HVal[_axis]);//debug

        break;
        case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
            responseData = Synta_hexToLong(packetIn);
            if (responseData > 650L){
                responseData = 650; //minimum speed to prevent timer overrun
            }
            _IVal[_axis]= responseData; //set the speed container (convert string to long first)
                       //Serial.printf("Ival %i\r",_IVal[_axis]);//debug
                       calculateRate(_axis); //Used to convert speed into the number of 0.5usecs per step. Result is stored in TimerOVF
            //responseData = 0;
            Serial.printf("=\r");
            
        break;
        case 'E': //set the current position, return empty response
            _jVal[_axis]= Synta_hexToLong(packetIn); //set the current position (used to sync to what EQMOD thinks is the current position at startup
            //responseData = 0;
            Serial.printf("=\r");
                                  // Serial.printf("jval %i\r",_jVal[_axis]);//debug

        break;
        case 'F': //Enable the motor driver, return empty response
            motorEnable(); //This enables the motors - gives the motor driver board power
            //responseData = 0;
            Serial.printf("=\r");
        break;
        default: //Return empty response (deals with commands that don't do anything before the response sent (i.e 'J'), or do nothing at all (e.g. 'M', 'L') )
            //responseData = 0;
            Serial.printf("=\r");
        break;
    }
    if(command == 'J'){ //J tells
        if(_GVal[_axis] & 1){
                                         //  Serial.printf("slew Gval %i\r",_GVal[_axis]);//debug

      //This is the funtion that enables a slew type move.
            slewMode(); //Slew type
        } else {
                                                //   Serial.printf("goto Gval %i\r",_GVal[_axis]);//debug

      //This is the function for goto mode. You may need to customise it for a different motor driver
            gotoMode(); //Goto Mode
        }
    }   
}
    

void configureTimer(){
    unsigned long rate;

    //interruptions
    RCONbits.IPEN = 1;					// Enable HP/LP interrupts
    INTCONbits.GIEH = 1;				// Enable HP interrupts
    INTCONbits.GIEL = 1;				// Enable LP interrupts
    INTCONbits.RBIE = INT_DISABLE; //RB4 Ã  7 => ST4 guiding  need some test and validation
    // we use timer3 and timer1  in count/compare mode. when timer registers (TMR) equals the value setted
    // in Compare register (CCPR), then the timer is zeroed and count again from 0 and an interrupt is raised
    // in the interrupt we will toggle the stepping output. since the timer is automatically (hardware) zeroed,
    // the only latency seen will be due to the if test (mainly this will be a pure delay at the first interrupt occurrence
    // then we can assume that between to output toggling the time will be exactly equal to the one setted in register
    // anyway if there is some jitter due to the if test in interrupt,this will be only "noise" in the exact stepping moment
    // hence we will stay at the rate programmed (no drift)
    PIE2bits.CCP2IE = INT_DISABLE; // CCP2=> DEC 
    PIE1bits.CCP1IE = INT_DISABLE; // CCP1=> RA
        
    timerCountRate = 150000;
//Set prescaler to F_CPU/8 //DEC
    T3CON = 0b10111000;
    IPR2bits.CCP2IP = INT_HIGH_PRIORITY; //is this mandatory? maybe LOW_PRIORITY is OK
    CCP2CON = 0b00001011;
    PIR2bits.CCP2IF = 0;
    TMR3H = 0; // timer reset
    TMR3L = 0;
//Set prescaler to F_CPU/8 //RA
    T1CON = 0b10110000;
    IPR1bits.CCP1IP = INT_HIGH_PRIORITY; //is this mandatory? maybe LOW_PRIORITY is OK
    CCP1CON = 0b00001011;
    PIR1bits.CCP1IF = 0;
    TMR1H = 0; // timer reset
    TMR1L = 0;
    rate = timerCountRate * 650L; //min rate of just under the sidereal rate
    rate /= _bVal[_axis];
    maxTimerOVF = 10 * rate; //corrects for timer count rate being set to 1/10th of the required to prevent numbers getting too big for an unsigned long.

    CCPR2H =  high8(maxTimerOVF);//initialize the compare registers
    CCPR2L =  low8(maxTimerOVF);
    CCPR1H =  high8(maxTimerOVF);
    CCPR1L =  low8(maxTimerOVF);
    T3CONbits.TMR3ON = ON; //start counting (but compare interrupt is not enabled here, only when motor starts)
    T1CONbits.TMR1ON = ON;
    #ifdef HAVEST4
    INTCONbits.RBIE = INT_ENABLE; //RB4 Ã  7 => ST4 guiding  need some test and validation
    INTCONbits.RBIF = 0;
    #endif
    #ifdef HAVEENCODER
    INTCONbits.RBIE = INT_ENABLE; //RB4 Ã  7 => ST4 guiding  need some test and validation
    INTCONbits.RBIF = 0;
    #endif

}



void setup() {
byte i;
Synta_init(1281, 9024000, 64933, 16, 50133); //EQ6 parameters (care: gval for the configuration 1/16 step to 1/2 is 8, not 16
//Synta synta(1281, 9024000, 64933, 16, 50133, 1); //care is to be taken with bval..sometimes roounding can wrongly affect the value 
//Synta synta(1281, 4512000, 32466, 16, 25067, 1);//this case will give ival = 619 in eqmod... so bval sould be 32467 instead


//div_scalar = _scalar/2;

pinMode(USERLED,OUTPUT);

for( i = 0;i < 2;i++){ //for each motor...
    pinMode(enablePin[i],OUTPUT); //enable the Enable pin
    digitalWrite(enablePin[i],HIGH); //IC disabled
    pinMode(stepPin[i],OUTPUT); //enable the step pin
    digitalWrite(stepPin[i],LOW); //default is low
    pinMode(dirPin[i],OUTPUT); //enable the direction pin
    digitalWrite(dirPin[i],LOW); //default is low
    #if defined(USEHIGHSPEEDMODE)
    pinMode(modePins[i][0],OUTPUT); //enable the mode pins
    pinMode(modePins[i][1],OUTPUT); //enable the mode pins
    digitalWrite(modePins[i][0],HIGH); //default is low 1/16 or 1/32 steps
    digitalWrite(modePins[i][1],HIGH); //default is high
    #endif
    pinMode(resetPin[i],OUTPUT); //enable the reset pin
    digitalWrite(resetPin[i],LOW); //active low reset
    delay(1); //allow ic to reset
    digitalWrite(resetPin[i],HIGH); //complete reset
    #ifdef HAVEST4
    pinMode(ST4Pin[i],INPUT); //enable the ST4 pin
    pinMode(ST4Pin[i+1],INPUT); //enable the ST4 pin
    #endif
   #ifdef HAVEENCODER
    pinMode(ST4Pin[i],INPUT); //enable the ST4 pin
    pinMode(ST4Pin[i+1],INPUT); //enable the ST4 pin
    #endif

}
// start Serial port:
Serial.begin(9600); //SyncScan runs at 9600Baud, use a serial port of your choice 
while(Serial.available()){
    Serial.read(); //Clear the buffer
}
configureTimer(); //setup the motor pulse timers.

}

void loop() {
    char recievedChar;
    char decoded;
    char decodedPacket[11];
    int time = 0;
    //temporary store for completed command ready to be processed
//    Serial.printf("%u\r",div_scalar);
//    Serial.printf("%i\r",maxTimerOVF);
    if (Serial.available()) { //is there a byte in buffer
        digitalWrite(USERLED,LOW); //Turn on the LED to indicate activity.
        recievedChar = Serial.read(); //get the next character in buffer
        decoded = Synta_recieveCommand(decodedPacket,recievedChar);//once full command packet recieved, decodedPacket returns either error packet, or valid packet
        time++;
        if (decoded == 1){ //Valid Packet
            decodeCommand(_command,decodedPacket); //decode the valid packet
        } else if (decoded == -1){ //Error
            Serial.printf(decodedPacket); //send the error packet (recieveCommand() already generated the error packet, so just need to send it)
        } //otherwise command not yet fully recieved, so wait for next byte
    } 
     if (time > 500){
    digitalWrite(USERLED,HIGH); //Turn off the LED to indicate activity.
    time = 0;
}  
}
    
    
void userhighinterrupt(){
    if (PIE1bits.CCP1IE && PIR1bits.CCP1IF){ //CCP1 =RA =Timer1 on steppe
        PORTCbits.RC0=1;
        delayMicroseconds(PULSEWIDTH);
        PORTCbits.RC0=0;
        PIR1bits.CCP1IF=0;
        checkIfDone(0);
    } else if (PIE2bits.CCP2IE && PIR2bits.CCP2IF){ //CCP2 =DEC on stepper
        PORTCbits.RC1=1;
        delayMicroseconds(PULSEWIDTH);
        PORTCbits.RC1=0;
        PIR2bits.CCP2IF=0;
        checkIfDone(1);
    }    
}

void userinterrupt(){
    #ifdef HAVEST4

    // RA guiding: is low priority so far, but need to check if either guiding should be high priority
    // or , why not, tracking and slewing be low priority...
    boolean RAp,RAm;
    boolean DEp,DEm;
    static s16 RAcorrection=0;
    static s16 DEcorrection= 0;
    if (INTCONbits.RBIE && INTCONbits.RBIF){ //get a signal on either RA+,RA-, DE+, DE-
        RAp = PORTBbits.RB7; //RA+
        RAm = PORTBbits.RB6; //RA-
        DEp = PORTBbits.RB5; //DEC+ au cas Ou
        DEm = PORTBbits.RB4; //DEC- au cas ou 
        if (RAp) // RA + we move faster
            RAcorrection = timerOVF[0]>>rate_modifier; //(correction by 0.25 ou 0.5)
        else if (RAm)//RA- slower
            RAcorrection = -(timerOVF[0]>>rate_modifier); //(correction by 0.25 ou 0.5)
        else //no RA correction
            RAcorrection = 0;
        CCPR1L =  low8(timerOVF[0]+RAcorrection);
        CCPR1H = high8(timerOVF[0]+RAcorrection);
        DEcorrection = timerOVF[1]>>rate_modifier;
        CCPR2L =  low8(DEcorrection);
        CCPR2H = high8(DEcorrection);
        if (DEp){ // DEC + we move at 0.25 or 0.5 sideral rate, toward west
            PIE2bits.CCP2IE = INT_ENABLE;
            digitalWrite(dirPin[1],LOW); //default is low (DE+
        }else if (DEm){//DEC- toward east
            PIE2bits.CCP2IE = INT_ENABLE;
            digitalWrite(dirPin[1],HIGH); //default is low (DE-
        }else //no DEC correction
            PIE2bits.CCP2IE = INT_DISABLE;
        INTCONbits.RBIF = 0;
    }
   #endif
   
   #ifdef HAVEENCODER
   //stuff to decode quadrature encoders 
   //see here fore example http://playground.arduino.cc/Main/RotaryEncoders#Example4
   #endif
   
 
}
