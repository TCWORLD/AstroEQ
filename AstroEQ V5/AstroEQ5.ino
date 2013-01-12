/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Syntia
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts (Not EQ3-2, they have a different gear ratio)
 
  Current Verison: 5.0
*/
//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
//Just in case it was defined somewhere else, we cant be having that.
#ifdef USEHIGHSPEEDMODE
#undef USEHIGHSPEEDMODE
#endif


#include "synta.h" //Synta Communications Protocol.


//The system needs to be configured to suit your mount.
//Either use the instructions below and calculate the values yourself,
//or use the "Initialisation Variable Calculator.xlsx" file.
//
// The line to configure the mount follows these instructions!
//
//
//  Format:
//
//Synta synta( e [1281],  a,  b,  g [16],  s,  scalar);
//
// e = version number (this doesnt need chaning)
//
// a = number of steps per complete axis revolution. 
// aVal = 64 * motor steps per revolution * ratio
// where ratio is the gear ratio from motor to axis. E.g. for an HEQ5 mount, this is 705
// and motor steps is how many steps per one revolution of the motor. E.g. a 1.8degree stepper has 200 steps/revolution
//
// b = sidereal rate. This again depends on number of microsteps per step. 
// bVal = 620 * aVal / 86164.0905   (round UP to nearest integer)
//
// g = high speed multiplyer. This is the EQMOD recommended default. In highspeed mode, the step rate will be 16 times larger.
//
// s = steps per worm revolution
// sVal = aVal / Worm Gear Teeth
// where worm gear teeth is how many teeth on the main worm gear. E.g. for an HEQ5 this is 135
//
// scalar = reduces the number reported to EQMOD, whilst not affecting mount behavior.
// This is such that: aVal/scalar < 10000000 (ten million). The max allowed is 15. If a number larger than this is required, let me know and I will see what can be done.
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
// EQ4/5 Mount upgraded with the Dual motor kit (non Goto) -- This is the only one I have tested, the others are theoretical
// aVal = 26542080
// bVal = 190985
// sVal = 184320
// scalar = 10
//
// EQ3 Mount upgraded with the Dual motor kit (non Goto)
// aVal = 23961600
// bVal = 172418
// sVal = 184320
// scalar = 10
//
// Other mounts:
// aVal = 64 * motor steps per revolution * ratio
// bVal = 620 * aVal / 86164.0905   (round to nearest whole number)
// sVal = aVal / Worm Gear Teeth   (round to nearest whole number)
// scalar = depends on aVal, see notes below
//
//Create an instance of the mount
//If aVal is roughly greater than 10000000 (ten million), the number needs to be scaled down by some factor
//to make it smaller than that limit. In my case I have chosen 10 as it nicely scales the
//a and s values. Hence below, I have set scalar to 10.




//This is the initialistation code. If using the excel file, replace this line with what it gives.
//                    aVal    bVal      sVal  scalar

//Example for same ratio per axis
Synta synta(1281, 13271040, 95493, 16, 92160, 3);
//Example for different ratio per axis (see excel file).
//Synta synta(1281, 9024000, 9011200, 64933, 64841, 62667, 62578, 16, 1);

//These allow you to configure the speed at which the mount moves when executing goto commands. (Lower is faster)
//It can be calculated from the value given for speed how fast the mount will move:
//seconds per revolution = speed * aVal / bVal
//Or in terms of how many times the sidereal rate it will move
//ratio = 86164.0905 * (bVal / (aVal * speed))
//
//When changing the speed, change the number below, ensuring to suffix it with the letter 'L'
#define NORMALGGOTOSPEED 320L //This is defined as Speed with the letter 'L' appended
#define FASTGOTOSPEED 176L

//This needs to be uncommented for those using the Pololu A4983 Driver board.
//#define POLOLUDRIVER 1

//Define the two axes (swap the 0 and 1 if R.A. and Dec. motors are reversed)
#define RA 0
#define DC 1

//Uncomment the following #define to use a Beta function which increases motor torque when moving at highspeed by switching from 16th ustep to half stepping.
//Note, this is very much a Beta, and requires mode pins M2 and M0 of the DRV8824 chips to be connected to the arduino. 
//It seems to work, but it isn't fully tested, so is turned off by default.

//#define USEHIGHSPEEDMODE 1


//-------------------------------------------------------------
//DO NOT EDIT ANYTHING BELOW THIS LINE-------------------------
//-------------------------------------------------------------

unsigned long gotoPosn[2] = {0,0}; //where to slew to
unsigned int timerOVF[2][64]; //Clock section of each of the 64 microsteps
unsigned long timerCountRate;
unsigned long maxTimerOVF; //Minimum allowable clock count

#define PULSEWIDTH16 64
#define PULSEWIDTH2 8

#ifdef USEHIGHSPEEDMODE
#define PULSEWIDTH16F 8
#define PULSEWIDTH2F 1
#endif

#if defined(__AVR_ATmega162__)

//Pins
const char statusPin = 13;
const char resetPin[2] = {15,14};
const char errorPin[2] = {2,6};
const char dirPin[2] = {3,7};
const char enablePin[2] = {4,8};
const char stepPin[2] = {5,30};
const char oldStepPin1 = 9; //Note this has changed due to a mistake I had made originally. The original will be set as an input so hardware can be corrected by shorting with new pin
const char modePins[2][2] = {{16,17},{19,18}};
//Used for direct port register write. Also determines which hardware
#define STEP1PORT PORTD
#define _STEP1_HIGH 0b00010000
#define _STEP1_LOW 0b11101111
#define STEP2PORT PORTE
#define _STEP2_HIGH 0b00000100
#define _STEP2_LOW 0b11111011

boolean highSpeed[2] = {false,false}; //whether number of microsteps is dropped to 8 due to high speed (to increase Torque)
byte distributionWidth[2] = {64,64};

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

//Pins
const char statusPin = 13;
const char resetPin[2] = {A1,A0};
const char errorPin[2] = {2,6};
const char dirPin[2] = {3,7};
const char enablePin[2] = {4,8};
const char stepPin[2] = {5,12};
const char oldStepPin1 = 9; //Note this has changed due to a mistake I had made originally. The original will be set as an input so hardware can be corrected by shorting with new pin
//Used for direct port register write. Also determines which hardware
#define STEP1PORT PORTE
#define _STEP1_HIGH 0b00001000
#define _STEP1_LOW 0b11110111
#define STEP2PORT PORTB
#define _STEP2_HIGH 0b01000000
#define _STEP2_LOW 0b10111111

#ifdef USEHIGHSPEEDMODE
const char modePins[2][2] = {{16,17},{19,18}}; // {RA{Mode0,Mode2}, DEC{Mode0,Mode2}} Modepins, Mode1=leave unconnected.
boolean highSpeed[2] = {false,false}; //whether number of microsteps is dropped to 8 due to high speed (to increase Torque) -
                                      //This is only available on Atmega162 as not included with original hardware.
                                      //If you want to use it, uncomment the line: '#define USEHIGHSPEEDMODE 1'
                                      //and connect the mode pins of the DRV8824PWP chips to the pins shown
#endif
byte distributionWidth[2] = {64,64};

#endif

#ifdef POLOLUDRIVER
#define MODE0STATE16 HIGH
#define MODE0STATE2 LOW
#else
#define MODE0STATE16 LOW
#define MODE0STATE2 HIGH
#endif
#define MODE2STATE16 HIGH
#define MODE2STATE2 LOW

void setup()
{
  pinMode(oldStepPin1,INPUT); //Set to input for ease of hardware change
  digitalWrite(oldStepPin1,LOW); //disable internal pullup
  pinMode(statusPin,OUTPUT);
  for(byte i = 0;i < 2;i++){ //for each motor...
    pinMode(errorPin[i],INPUT); //enable the Error pin
    pinMode(enablePin[i],OUTPUT); //enable the Enable pin
    digitalWrite(enablePin[i],HIGH); //IC disabled
    pinMode(stepPin[i],OUTPUT); //enable the step pin
    digitalWrite(stepPin[i],LOW); //default is low
    pinMode(dirPin[i],OUTPUT); //enable the direction pin
    digitalWrite(dirPin[i],LOW); //default is low
#if defined(__AVR_ATmega162__) || defined(USEHIGHSPEEDMODE)
    pinMode(modePins[i][0],OUTPUT); //enable the mode pins
    pinMode(modePins[i][1],OUTPUT); //enable the mode pins
    digitalWrite(modePins[i][0],MODE0STATE16); //default is low
    digitalWrite(modePins[i][1],MODE2STATE16); //default is high
#endif
    pinMode(resetPin[i],OUTPUT); //enable the reset pin
    digitalWrite(resetPin[i],LOW); //active low reset
    delay(1); //allow ic to reset
    digitalWrite(resetPin[i],HIGH); //complete reset
  }
    
  // start Serial port:
  Serial.begin(9600); //SyncScan runs at 9600Baud, use a serial port of your choice 
  while(Serial.available()){
    Serial.read(); //Clear the buffer
  }

  configureTimer(); //setup the motor pulse timers.
}

void loop(){
  static unsigned long lastMillis = millis();
  static boolean isLedOn = false;
  char decodedPacket[11]; //temporary store for completed command ready to be processed
  if (Serial.available()) { //is there a byte in buffer
    digitalWrite(statusPin,HIGH); //Turn on the LED to indicate activity.
    char recievedChar = Serial.read(); //get the next character in buffer
    char decoded = synta.recieveCommand(decodedPacket,recievedChar); //once full command packet recieved, decodedPacket returns either error packet, or valid packet
    if (decoded == 1){ //Valid Packet
      decodeCommand(synta.command(),decodedPacket); //decode the valid packet
    } else if (decoded == -1){ //Error
      Serial.print(decodedPacket); //send the error packet (recieveCommand() already generated the error packet, so just need to send it)
    } //otherwise command not yet fully recieved, so wait for next byte
    lastMillis = millis();
    isLedOn = true;
  }
  if (isLedOn && (millis() > lastMillis + 20)){
    isLedOn = false;
    digitalWrite(statusPin,LOW); //Turn LED back off after a short delay.
  }
}

void decodeCommand(char command, char* packetIn){ //each command is axis specific. The axis being modified can be retrieved by calling synta.axis()
  char response[11]; //generated response string
  unsigned long responseData; //data for response
  
  switch(command){
    case 'e': //readonly, return the eVal (version number)
        responseData = synta.cmd.eVal(synta.axis()); //response to the e command is stored in the eVal function for that axis.
        break;
    case 'a': //readonly, return the aVal (steps per axis)
        responseData = synta.cmd.aVal(synta.axis()); //response to the a command is stored in the aVal function for that axis.
        responseData /= synta.scalar();
        break;
    case 'b': //readonly, return the bVal (sidereal rate)
        responseData = synta.cmd.bVal(synta.axis()); //response to the b command is stored in the bVal function for that axis.
        responseData /= synta.scalar();
        if (synta.cmd.bVal(synta.axis()) % synta.scalar()){
          responseData++; //round up
        }
        break;
    case 'g': //readonly, return the gVal (high speed multiplier)
        responseData = synta.cmd.gVal(synta.axis()); //response to the g command is stored in the gVal function for that axis.
        break;
    case 's': //readonly, return the sVal (steps per worm rotation)
        responseData = synta.cmd.sVal(synta.axis()); //response to the s command is stored in the sVal function for that axis.
        responseData /= synta.scalar();
        break;
    case 'f': //readonly, return the fVal (axis status)
        responseData = synta.cmd.fVal(synta.axis()); //response to the f command is stored in the fVal function for that axis.
        break;
    case 'j': //readonly, return the jVal (current position)
        responseData = synta.cmd.jVal(synta.axis()); //response to the j command is stored in the jVal function for that axis.
        break;
    case 'K': //stop the motor, return empty response
    
    
        motorStop(synta.axis()); //No specific response, just stop the motor (Feel free to customise motorStop function to suit your needs
      
      
        responseData = 0;
        break;
    case 'G': //set mode and direction, return empty response
        synta.cmd.GVal(synta.axis(), (packetIn[0] - 48)); //Store the current mode for the axis
        synta.cmd.dir(synta.axis(),(packetIn[1] - 48)); //Store the current direction for that axis
        responseData = 0;
        break;
    case 'H': //set goto position, return empty response (this sets the number of steps to move from cuurent position if in goto mode)
        synta.cmd.HVal(synta.axis(), synta.hexToLong(packetIn)); //set the goto position container (convert string to long first)
        responseData = 0;
        break;
    case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
        responseData = synta.hexToLong(packetIn);
        if (responseData > 650L){
          responseData = 650; //minimum speed to prevent timer overrun
        }
        synta.cmd.IVal(synta.axis(), responseData); //set the speed container (convert string to long first)
        
        //This will be different if you use a different motor code
        calculateRate(synta.axis()); //Used to convert speed into the number of 0.5usecs per step. Result is stored in TimerOVF
  
  
        responseData = 0;
        break;
    case 'E': //set the current position, return empty response
        synta.cmd.jVal(synta.axis(), synta.hexToLong(packetIn)); //set the current position (used to sync to what EQMOD thinks is the current position at startup
        responseData = 0;
        break;
    case 'F': //Enable the motor driver, return empty response


        motorEnable(); //This enables the motors - gives the motor driver board power


        responseData = 0;
        break;
    default: //Return empty response (deals with commands that don't do anything before the response sent (i.e 'J'), or do nothing at all (e.g. 'M', 'L') )
        responseData = 0;
        break;
  }
  
  synta.assembleResponse(response, command, responseData); //generate correct response (this is required as is)
  Serial.print(response); //send response to the serial port
  
  if(command == 'J'){ //J tells
    if(synta.cmd.GVal(synta.axis()) & 1){
      
      //This is the funtion that enables a slew type move.
      slewMode(); //Slew type
      
      
    } else {
      
      //This is the function for goto mode. You may need to customise it for a different motor driver
      gotoMode(); //Goto Mode
      
      
    }
  }
}



//Calculates the rate based on the recieved I value 
void calculateRate(byte motor){
  
  //seconds per step = IVal / BVal; for lowspeed move
  //or seconds per step = IVal / (BVal * gVal); for highspeed move 
  //
  //whether to use highspeed move or not is determined by the GVal;
  //
  //clocks per step = timerCountRate * seconds per step
  
  unsigned long rate;
  unsigned long remainder;
  float floatRemainder;
  unsigned long divisor = synta.cmd.bVal(motor);
  if((synta.cmd.GVal(motor) == 2)||(synta.cmd.GVal(motor) == 1)){ //Normal Speed
#if defined(USEHIGHSPEEDMODE)
    //check if at very high speed
    if (synta.cmd.IVal(motor) < 30){
      highSpeed[motor] = true;
      distributionWidth[motor] = 8; //There are 8 times fewer steps over which to distribute extra clock cycles
    } else {
      highSpeed[motor] = false;
      distributionWidth[motor] = 64;
    }
#endif
  } else if ((synta.cmd.GVal(motor) == 0)||(synta.cmd.GVal(motor) == 3)){ //High Speed
    divisor *= synta.cmd.gVal(motor);
#if defined(USEHIGHSPEEDMODE)
    //check if at very high speed
    if (synta.cmd.IVal(motor) < (30 * synta.cmd.gVal(motor))){
      highSpeed[motor] = true;
      distributionWidth[motor] = 8; //There are 8 times fewer steps over which to distribute extra clock cycles
    } else {
      highSpeed[motor] = false;
      distributionWidth[motor] = 64;
    }
#endif
  }
  rate = timerCountRate * synta.cmd.IVal(motor);
  //When dividing a very large number by a much smaller on, float accuracy is abismal. So firstly we use integer math to split the division into quotient and remainder
  remainder = rate % divisor; //Calculate the remainder
  rate /= divisor; //Calculate the quotient
  //Then convert the remainder into a decimal number (division of a small number by a larger one, improving accuracy)
  floatRemainder = (float)remainder/(float)divisor; //Convert the remainder to a decimal.
  
  rate *= 10; //corrects for timer count rate being set to 1/10th of the required to prevent numbers getting too big for an unsigned long.
  floatRemainder *= 10;
  
  //If there is now any integer part of the remainder after the multiplication by 10, it needs to be added back on to the rate and removed from the remainder.
  remainder = (unsigned long)floatRemainder; //first work out how much to be added
  //Then add it to the rate, and remove it from the decimal remainder
  rate += remainder;
  floatRemainder -= (float)remainder;
  
  //Multiply the remainder by distributionWidth to work out an approximate number of extra clocks needed per full step (each step is 'distributionWidth' microsteps)
  floatRemainder *= (float)distributionWidth[motor]; 
  //This many extra microseconds are needed:
  remainder = (unsigned long)floatRemainder; 
  
  //Now truncate to an unsigned int with a sensible max value (the int is to avoid register issues with the 16 bit timer)
  if(rate > 64997UL){
    rate = 64997UL;
  } else if (rate < 36UL) {
    rate = 36L;
  }
  for (int i = 0; i < distributionWidth[motor]; i++){
    timerOVF[motor][i] = rate - 1; //Subtract 1 as timer is 0 indexed.
  }
  
  //evenly distribute the required number of extra clocks over the full step.
  for (int i = 0; i < remainder; i++){
    float distn = i;
    distn *= (float)distributionWidth[motor];
    distn /= (float)remainder;
    byte index = (byte)ceil(distn);
    timerOVF[motor][index] += 1;
  }
}

void motorEnable(){
  digitalWrite(enablePin[synta.axis()],LOW);
  synta.cmd.FVal(synta.axis(),1);
}

void slewMode(){
  motorStart(synta.axis(), 30/synta.scalar()); //Begin PWM
}

void gotoMode(){
  if (synta.cmd.HVal(synta.axis()) < (140/synta.scalar())){
    if (synta.cmd.HVal(synta.axis()) < 2){
      synta.cmd.HVal(synta.axis(),2);
    }
    //To short of a goto to use the timers, so do it manually
    accellerate( (synta.cmd.HVal(synta.axis()) / 2) * synta.scalar(), synta.axis());
    decellerate( (synta.cmd.HVal(synta.axis()) / 2) * synta.scalar(), synta.axis());
    
    synta.cmd.jVal(synta.axis(), (synta.cmd.jVal(synta.axis()) + (synta.cmd.stepDir(synta.axis()) * synta.cmd.HVal(synta.axis())) ) );
  } else {
    gotoPosn[synta.axis()] = synta.cmd.jVal(synta.axis()) + (synta.cmd.stepDir(synta.axis()) * synta.cmd.HVal(synta.axis())); //current position + distance to travel
    
    if (synta.cmd.HVal(synta.axis()) < (200/synta.scalar())){
      synta.cmd.IVal(synta.axis(), NORMALGGOTOSPEED);
      calculateRate(synta.axis());
      motorStart(synta.axis(),20/synta.scalar()); //Begin PWM
    } else {
      synta.cmd.IVal(synta.axis(), FASTGOTOSPEED); //Higher speed goto
      calculateRate(synta.axis());
      motorStart(synta.axis(),60/synta.scalar()); //Begin PWM
    }
    synta.cmd.gotoEn(synta.axis(),1);
  }
}

void motorStart(byte motor, byte steps){
  digitalWrite(dirPin[motor],synta.cmd.dir(motor)); //set the direction
  synta.cmd.stopped(motor, 0);
  accellerate(steps * synta.scalar(),motor); //accellerate to calculated rate in given number of steps steps
  synta.cmd.jVal(motor, (synta.cmd.jVal(motor) + (synta.cmd.stepDir(motor) * steps) ) ); //update position to account for accelleration
  if(motor){
    ICR1 = timerOVF[motor][0];
    TCNT1 = 0;
    TCCR1A |= (1<<COM1B1); //Set port operation to fast PWM
#if defined(__AVR_ATmega162__)
    TIMSK |= (1<<TOIE1); //Enable timer interrupt
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    TIMSK1 |= (1<<TOIE1); //Enable timer interrupt
#endif
  } else {
    ICR3 = timerOVF[motor][0];
    TCNT3 = 0;
    TCCR3A |= (1<<COM3A1); //Set port operation to fast PWM
#if defined(__AVR_ATmega162__)
    ETIMSK |= (1<<TOIE3); //Enable timer interrupt
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    TIMSK3 |= (1<<TOIE3); //Enable timer interrupt
#endif
  }
}

void motorStop(byte motor){
  motorStop(motor, 60/synta.scalar()); //Default number of steps. Replaces function prototype from Version < 5.0
}

void motorStop(byte motor, byte steps){
  if (!synta.cmd.stopped(motor)){
    if(motor){
      TCCR1A &= ~(1<<COM1B1); //Set port operation to Normal
#if defined(__AVR_ATmega162__)
      TIMSK &= ~(1<<TOIE1); //Disable timer interrupt
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      TIMSK1 &= ~(1<<TOIE1); //Disable timer interrupt
#endif
    } else {
      TCCR3A &= ~(1<<COM3A1); //Set port operation to Normal
#if defined(__AVR_ATmega162__)
      ETIMSK &= ~(1<<TOIE3); //Disable timer interrupt
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      TIMSK3 &= ~(1<<TOIE3); //Disable timer interrupt
#endif
    }
    //Only decellerate if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
    if (synta.cmd.gotoEn(motor)){
      synta.cmd.gotoEn(motor,0); //Cancel goto mode
    }
    decellerate(steps * synta.scalar(),motor);
    synta.cmd.jVal(motor, (synta.cmd.jVal(motor) + (synta.cmd.stepDir(motor) * steps) ) );
    synta.cmd.stopped(motor,1); //mark as stopped
  }
}

void decellerate(byte decellerateSteps, byte motor){
#if defined(USEHIGHSPEEDMODE)
  if(highSpeed[motor]){
    //Disable Highspeed mode
    highSpeed[motor] = false;
    digitalWrite(modePins[motor][0],MODE0STATE16);
    digitalWrite(modePins[motor][1],MODE2STATE16);
    if(motor){
      if (synta.cmd.bVal(synta.axis()) < 160000){
        OCR1B = PULSEWIDTH2; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/8
        TCCR1B &= ~((1<<CS12) | (1<<CS10)); //0x0
        TCCR1B |= (1<<CS11);//1
      } else {
        OCR1B = PULSEWIDTH16; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/1
        TCCR1B &= ~((1<<CS12) | (1<<CS11));//00x
        TCCR1B |= (1<<CS10);//1
      }
    } else {
      if (synta.cmd.bVal(synta.axis()) < 160000){
        OCR3A = PULSEWIDTH2; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/8
        TCCR3B &= ~((1<<CS32) | (1<<CS30)); //0x0
        TCCR3B |= (1<<CS31);//1
      } else {
        OCR3A = PULSEWIDTH16; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/1
        TCCR3B &= ~((1<<CS32) | (1<<CS31));//00x
        TCCR3B |= (1<<CS30);//1
      }
    }
  }
#endif
  //x steps to slow down to maxTimerOVF
  speedCurve(timerOVF[motor][0], 1, decellerateSteps, motor);
  //Now at minimum speed, so safe to stop.
}
  
void accellerate(byte accellerateSteps, byte motor){
  //x steps to speed up to timerOVF[synta.axis()]
  speedCurve(maxTimerOVF, -1, accellerateSteps, motor);
  //Now at speed, so check highspeed mode and then return. Timers will be used from now on.
#if defined(USEHIGHSPEEDMODE)
  if(highSpeed[motor]){
    //Enable Highspeed mode
    digitalWrite(modePins[motor][0],MODE0STATE2);
    digitalWrite(modePins[motor][1],MODE2STATE2); 
    if(motor){
      if (synta.cmd.bVal(synta.axis()) < 160000){
        OCR1B = PULSEWIDTH2F; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/64
        TCCR1B &= ~(1<<CS12); //0
        TCCR1B |= ((1<<CS11) | (1<<CS10));//x11
    
      } else {
        OCR1B = PULSEWIDTH16F; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/8
        TCCR1B &= ~((1<<CS12) | (1<<CS10)); //0x0
        TCCR1B |= (1<<CS11);//1
      }
    } else {
      if (synta.cmd.bVal(synta.axis()) < 160000){
        OCR3A = PULSEWIDTH2F; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/64
        TCCR3B &= ~(1<<CS32); //0
        TCCR3B |= ((1<<CS31) | (1<<CS30));//x11
      } else {
        OCR3A = PULSEWIDTH16F; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
        //Set prescaler to F_CPU/8
        TCCR3B &= ~((1<<CS32) | (1<<CS30)); //0x0
        TCCR3B |= (1<<CS31);//1
      }
    }
  }
#endif
}

void speedCurve(unsigned long currentSpeed, char dir, byte steps, byte motor){ //dir = -1 or +1. - means accellerate
  unsigned long speedPerStep = (maxTimerOVF - timerOVF[motor][0]); //maxTimerOVF is always largest
  speedPerStep /= steps;
  if (synta.cmd.bVal(motor) < 160000){//convert the number of clock cycles into uS.
    speedPerStep >>= 1;
  } else {
    speedPerStep >>= 4;
  }
  if (speedPerStep == 0){
    speedPerStep = 1; //just in case - prevents rounding from resulting in delay of 0.
  }
  unsigned int microDelay;
  if (synta.cmd.bVal(motor) < 160000){
    microDelay = currentSpeed >> 1; //number of uS to wait for speed (rate is in 1/2ths of a microsecond)
  } else {
    microDelay = currentSpeed >> 4; //number of uS to wait for speed (rate is in 1/16ths of a microsecond)
  }
  for(int i = 0;i < steps;i++){
    delayMicroseconds(microDelay);
    if(motor){ //Step
      writeSTEP2(HIGH);
      delayMicroseconds(5);
      writeSTEP2(LOW);
    } else {
      writeSTEP1(HIGH);
      delayMicroseconds(5);
      writeSTEP1(LOW);
    }
    microDelay += (dir * speedPerStep);
  }
}

//Timer Interrupt-----------------------------------------------------------------------------
void configureTimer(){
#if defined(__AVR_ATmega162__)
  TIMSK &= ~(1<<TOIE1); //disable timer so it can be configured
  ETIMSK &= ~(1<<TOIE3); //disable timer so it can be configured
  //Disable compare interrupt (only interested in overflow)
  TIMSK &= ~((1<<OCIE1A) | (1<<OCIE1B));
  ETIMSK &= ~((1<<OCIE3A) | (1<<OCIE3B));
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  TIMSK1 &= ~(1<<TOIE1); //disable timer so it can be configured
  TIMSK3 &= ~(1<<TOIE3); //disable timer so it can be configured
  //Disable compare interrupt (only interested in overflow)
  TIMSK1 &= ~((1<<OCIE1A) | (1<<OCIE1B) | (1<<OCIE1C));
  TIMSK3 &= ~((1<<OCIE3A) | (1<<OCIE3B) | (1<<OCIE3C));
#endif
  //set to fast PWM mode (1110)
  TCCR1A |= ((1<<WGM11));
  TCCR1A &= ~((1<<WGM10));
  TCCR1B |= ((1<<WGM13) | (1<<WGM12));
  TCCR3A |= ((1<<WGM31));
  TCCR3A &= ~((1<<WGM30));
  TCCR3B |= ((1<<WGM33) | (1<<WGM32));
  if (synta.cmd.bVal(synta.axis()) < 160000){
    timerCountRate = 200000;
    //Set prescaler to F_CPU/8
    TCCR1B &= ~((1<<CS12) | (1<<CS10)); //0x0
    TCCR1B |= (1<<CS11);//1
    TCCR3B &= ~((1<<CS32) | (1<<CS30)); //0x0
    TCCR3B |= (1<<CS31);//1
    OCR1B = PULSEWIDTH2; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
    OCR3A = PULSEWIDTH2;
  } else {
    timerCountRate = 1600000;
    //Set prescaler to F_CPU/1
    TCCR1B &= ~((1<<CS12) | (1<<CS11));//00x
    TCCR1B |= (1<<CS10);//1
    TCCR3B &= ~((1<<CS32) | (1<<CS31));//00x
    TCCR3B |= (1<<CS30);//1
    OCR1B = PULSEWIDTH16; //Width of step pulse (equates to ~4uS. DRV8824PWP specifies absolute minimum as ~2uS)
    OCR3A = PULSEWIDTH16;
  }
  unsigned long rate = timerCountRate * 650L; //min rate of just under the sidereal rate
  rate /= synta.cmd.bVal(synta.axis());
  maxTimerOVF = 10 * rate; //corrects for timer count rate being set to 1/10th of the required to prevent numbers getting too big for an unsigned long.
}

/*Timer Interrupt Vector*/
ISR(TIMER3_OVF_vect) {
  static byte timeSegment = 0;
  ICR3 = timerOVF[RA][timeSegment++]; //move to next pwm period
  if (timeSegment == distributionWidth[RA]){
    timeSegment = 0;
  }
  motorStep(RA);
}

/*Timer Interrupt Vector*/
ISR(TIMER1_OVF_vect) {
  static byte timeSegment = 0;
  ICR1 = timerOVF[DC][timeSegment++]; //move to next pwm period
  if (timeSegment == distributionWidth[DC]){
    timeSegment = 0;
  }
  motorStep(DC);
}

void motorStep(byte motor){
  static char divider[2] = {0};
  
  divider[motor] += synta.cmd.stepDir(motor);
  if(synta.cmd.stepDir(motor) < 0){
    if (divider[motor] < 0){
      divider[motor] = synta.scalar() - 1;
      checkIfDone(motor);
    }
  } else {
    if (divider[motor] >= synta.scalar()){
      divider[motor] = 0;
      checkIfDone(motor);
    }
  }
}

void checkIfDone(byte motor){
#if defined(USEHIGHSPEEDMODE)
  if(highSpeed[motor]){
    synta.cmd.jVal(motor, (synta.cmd.jVal(motor) + (8 * synta.cmd.stepDir(motor))));
  } else {
#else
  if(1){ //Always do the next line. Here to account for the extra close brace
  //} -> this one is supposed to be commented out! (keeps the IDE happy)
#endif
    synta.cmd.jVal(motor, (synta.cmd.jVal(motor) + synta.cmd.stepDir(motor)));
  }
  if(synta.cmd.gotoEn(motor)){
    long stepsLeft = gotoPosn[motor] - synta.cmd.jVal(motor);
    stepsLeft *= synta.cmd.stepDir(motor);
    if(stepsLeft <= (120/synta.scalar())){
      motorStop(motor, stepsLeft); //will decellerate the rest of the way.
    }
  }
}

void writeSTEP1(boolean bitValue){
  if (bitValue){
    STEP1PORT |= _STEP1_HIGH;
  } else {
    STEP1PORT &= _STEP1_LOW;
  }
}

void writeSTEP2(boolean bitValue){
  if (bitValue){
    STEP2PORT |= _STEP2_HIGH;
  } else {
    STEP2PORT &= _STEP2_LOW;
  }
}
//-------------------------------------------------------------------------------------------
#else
#error Unsupported Part! Please use an Arduino Mega, or ATMega162
#endif

