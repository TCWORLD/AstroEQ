/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Syntia
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts (Not EQ3-2, they have a different gear ratio)
 
  Current Verison: 4.0
*/

#include "synta.h"

//Synta synta(e,a,b,g,s,scalar);
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
// This is required if aVal is roughly greater than 10000000 (ten million). Numbers larger than this will overflow calculation limits and result in weird behaviour.
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
// scalar = 10   -> This is vital that it be set as 10!!!
//
// EQ3 Mount upgraded with the Dual motor kit (non Goto)
// aVal = 23961600
// bVal = 172418
// sVal = 184320
// scalar = 10   -> This is vital that it be set as 10!!!
//
// Other mounts:
// aVal = 64 * motor steps per revolution * ratio
// bVal = 620 * aVal / 86164.0905   (round UP to nearest integer)
// sVal = aVal / Worm Gear Teeth
// scalar = depends on aVal, see notes below
//
//Create an instance of the mount
//If aVal is roughly greater than 10000000 (ten million), the number needs to be scaled down by some factor
//to make it smaller than that limit. In my case I have chosen 10 as it nicely scales the
//a and s values. Hence below, I have set scalar to 10.

Synta synta(1281,26542080,190985,16,184320,10);

//Define the two axes (swap if RA and DEC motors are reversed)
#define RA 0
#define DC 1

unsigned long gotoPosn[2] = {0,0}; //where to slew to
unsigned int timerOVF[2];
unsigned long timerCountRate;
unsigned long minTimerOVF;

//Pins
const char resetPin[2] = {A1,A0};
const char errorPin[2] = {2,6};
const char dirPin[2] = {3,7};
const char enablePin[2] = {4,8};
const char stepPin[2] = {5,9};
//Used for direct port register write
#define STEP1PORT PORTE
#define _STEP1_HIGH 0b00001000
#define _STEP1_LOW 0b11110111
#define STEP2PORT PORTH
#define _STEP2_HIGH 0b01000000
#define _STEP2_LOW 0b10111111


void setup()
{
  for(byte i = 0;i < 2;i++){ //for each motor...
    pinMode(errorPin[i],INPUT); //enable the Error pin
    pinMode(enablePin[i],OUTPUT); //enable the Enable pin
    digitalWrite(enablePin[i],HIGH); //IC disabled
    pinMode(stepPin[i],OUTPUT); //enable the step pin
    digitalWrite(stepPin[i],LOW); //default is low
    pinMode(dirPin[i],OUTPUT); //enable the direction pin
    digitalWrite(dirPin[i],LOW); //default is low
    pinMode(resetPin[i],OUTPUT); //enable the reset pin
    digitalWrite(resetPin[i],LOW); //active low reset
    delay(1); //allow ic to reset
    digitalWrite(resetPin[i],HIGH); //complete reset
  }
    
  // start Serial port:
  Serial.begin(9600); //SyncScan runs at 9600Baud, use a serial port of your choice (Serial, Serial, Serial2, or Serial3)
  while(Serial.available()){
    Serial.read(); //Clear the buffer
  }

  Serial1.begin(115200);
  configureTimer(); //setup the motor pulse timers.
}

void loop(){
  char decodedPacket[11]; //temporary store for completed command ready to be processed
  if (Serial.available()) { //is there a byte in buffer
    char recievedChar = Serial.read(); //get the next character in buffer
    Serial1.print(recievedChar);
    if(recievedChar == 'T'){
      testMode();
      return;
    } else if(recievedChar == 'S'){
      stepMode();
      return;
    }
    char decoded = synta.recieveCommand(decodedPacket,recievedChar); //once full command packet recieved, decodedPacket returns either error packet, or valid packet
    if (decoded == 1){ //Valid Packet
      decodeCommand(synta.command(),decodedPacket); //decode the valid packet
    } else if (decoded == -1){ //Error
      Serial.print(decodedPacket); //send the error packet
    } //otherwise command not yet fully recieved, so wait for next byte
  }
}

void testMode(){
  while(Serial.available()){
    Serial.read(); //Clear the buffer
  }
  while(1){
    delay(100);
    Serial.print('.');
    if (Serial.read()=='C') {
      while(Serial.available()){
        Serial.read(); //Clear the buffer
      }
      break;
    }
  }
  digitalWrite(enablePin[0],LOW);
  for(long i = 0L; i < 12800L;i++){
    delayMicroseconds(7990);
    writeSTEP1(HIGH);
    delayMicroseconds(10);
    writeSTEP1(LOW);
    long check = i % 64L;
    if(check == 0){
      char out[20] = {0};
      sprintf(out,"%ld\n",i);
      Serial.print(out);
    }
  }
}
void stepMode(){
  digitalWrite(enablePin[0],LOW);
  while(Serial.available()){
    Serial.read(); //Clear the buffer
  }
  while(1){
    delay(10);
    Serial.print('.');
    char bleh = Serial.read();
    if (bleh=='L') {
      while(Serial.available()){
        Serial.read(); //Clear the buffer
      }
      break;
    } else if (bleh=='C') {
      writeSTEP1(HIGH);
      delayMicroseconds(10);
      writeSTEP1(LOW);
    } else if (bleh=='R') {
      for(int i = 0;i<32;i++){
        Serial.print("boo\n");
        writeSTEP1(HIGH);
        delayMicroseconds(10);
        writeSTEP1(LOW);
        delayMicroseconds(1000);
      }
    }
  }
}

void decodeCommand(char command, char* packetIn){ //each command is axis specific. The axis being modified can be retrieved by calling synta.axis()
  char response[11]; //generated response string
  char check[100] = {0};
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
          responseData = 650L; //minimum speed to prevent timer overrun
        }
        synta.cmd.IVal(synta.axis(), responseData); //set the speed container (convert string to long first)
        
        //This will be different if you use a different motor code
        calculateRate(); //Used to convert speed into the number of 0.5usecs per step. Result is stored in TimerOVF
  
  
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
void calculateRate(){
  
  //seconds per step = IVal / BVal; for lowspeed move
  //or seconds per step = IVal / (BVal * gVal); for highspeed move 
  //
  //whether to use highspeed move or not is determined by the GVal;
  //
  //clocks per step = timerCountRate * seconds per step
  
  unsigned long rate;
  if((synta.cmd.GVal(synta.axis()) == 2)||(synta.cmd.GVal(synta.axis()) == 1)){ //Normal Speed
    rate = timerCountRate * synta.cmd.IVal(synta.axis());
    rate /= synta.cmd.bVal(synta.axis());
  } else if ((synta.cmd.GVal(synta.axis()) == 0)||(synta.cmd.GVal(synta.axis()) == 3)){ //High Speed
    rate = timerCountRate * synta.cmd.IVal(synta.axis());
    rate /= synta.cmd.bVal(synta.axis());
    rate /= synta.cmd.gVal(synta.axis());
  }
  rate *= 10; //corrects for timer count rate being set to 1/10th of the required to prevent numbers getting too big for an unsigned long.
  //Now truncate to an unsigned int with a sensible max value (the int is to avoid register issues with the 16 bit timer)
  if(rate < 539){ //We do 65535 - result as timer counts up from this number to 65536 then interrupts
    timerOVF[synta.axis()] = 64997L;
  } else if (rate > 65534L) {
    timerOVF[synta.axis()] = 1L;
  } else {
    timerOVF[synta.axis()] = 65535L - rate;
  }
}

void slewMode(){
  digitalWrite(dirPin[synta.axis()],synta.cmd.dir(synta.axis())); //set the direction
  accellerate(32,synta.axis()); //accellerate to calculated rate in 32 steps
  if(synta.axis()){
    TCNT4 = timerOVF[synta.axis()];
    TIMSK4 |= (1<<TOIE4); //Enable timer interrupt
  } else {
    TCNT3 = timerOVF[synta.axis()];
    TIMSK3 |= (1<<TOIE3); //Enable timer interrupt
  }
  synta.cmd.stopped(synta.axis(), 0);
}

void gotoMode(){
  digitalWrite(dirPin[synta.axis()],synta.cmd.dir(synta.axis())); //set the direction
  unsigned int temp = 0;
  if (synta.cmd.HVal(synta.axis()) < (40/synta.scalar())){
    synta.cmd.HVal(synta.axis(), (40/synta.scalar())); //This will cause a small error on VERY TINY Goto's though in reality should never be an issue.
  }
  gotoPosn[synta.axis()] = synta.cmd.jVal(synta.axis()) + (synta.cmd.stepDir(synta.axis()) * synta.cmd.HVal(synta.axis())); //current position
  
  if (synta.cmd.HVal(synta.axis()) < (100/synta.scalar())){
    synta.cmd.IVal(synta.axis(), 320L);
    calculateRate();
    accellerate(20,synta.axis()); //accellerate to calculated rate in 20 steps
  } else {
    synta.cmd.IVal(synta.axis(), 176L); //Higher speed slew
    calculateRate();
    accellerate(64,synta.axis()); //accellerate to calculated rate in 64 steps
  }
  synta.cmd.gotoEn(synta.axis(),1);
  if(synta.axis()){
    TCNT4 = timerOVF[synta.axis()];
    TIMSK4 |= (1<<TOIE4); //Enable timer interrupt
  } else {
    TCNT3 = timerOVF[synta.axis()];
    TIMSK3 |= (1<<TOIE3); //Enable timer interrupt
  }
  synta.cmd.stopped(synta.axis(),0);
}

void motorStop(byte motor){
  if(motor){ //switch off correct axis
    TIMSK4 &= ~(1<<TOIE4); //disable timer
  } else {
    TIMSK3 &= ~(1<<TOIE3); //disable timer
  }
  if (synta.cmd.gotoEn(motor)){
    synta.cmd.gotoEn(motor,0); //Cancel goto mode
    decellerate(20,motor); //last 20 steps are decelleration. motorStop() is called 20 steps early to account for this.
  } else {
    decellerate(100,motor); //decellerate in 100 steps.
  }
  synta.cmd.stopped(motor,1); //mark as stopped
}

void motorEnable(){
  digitalWrite(enablePin[synta.axis()],LOW);
  synta.cmd.FVal(synta.axis(),1);
}

void decellerate(byte decellerateSteps, byte motor){
  //x steps to slow down to minTimerOVF
  unsigned long rate = 65535L - timerOVF[motor]; //rate is the number of clocks between steps at maximum speed.
  unsigned long speedPerStep = (timerOVF[motor] - minTimerOVF);
  speedPerStep /= decellerateSteps;
  if (synta.cmd.bVal(motor) < 160000){//convert the number of clock cycles into uS.
    speedPerStep >>= 1;
  } else {
    speedPerStep >>= 4;
  }
  if (speedPerStep == 0){
    speedPerStep = 1; //just in case - prevents rounding from resulting in no accelleration at all.
  }
  unsigned int microDelay;
  if (synta.cmd.bVal(motor) < 160000){
    microDelay = rate >> 1; //number of uS to wait for current speed (rate is in 1/2ths of a microsecond)
  } else {
    microDelay = rate >> 4; //number of uS to wait for current speed (rate is in 1/16ths of a microsecond)
  }
  for(int i = 0;i < decellerateSteps;i++){
    delayMicroseconds(microDelay);
    if(synta.axis()){ //Step
      writeSTEP2(HIGH);
      delayMicroseconds(15);
      writeSTEP2(LOW);
    } else {
      writeSTEP1(HIGH);
      delayMicroseconds(15);
      writeSTEP1(LOW);
    }
    microDelay += speedPerStep;
  }
  //Now at minimum speed, so safe to stop.
}
  
void accellerate(byte accellerateSteps, byte motor){
  //x steps to speed up to timerOVF[synta.axis()]
  unsigned long rate = 65535L - minTimerOVF; //rate is the number of clocks between steps at minimum speed.
  unsigned long speedPerStep = (timerOVF[motor] - minTimerOVF);
  speedPerStep /= accellerateSteps;
  if (synta.cmd.bVal(motor) < 160000){//convert the number of clock cycles into uS.
    speedPerStep >>= 1;
  } else {
    speedPerStep >>= 4;
  }
  if (speedPerStep == 0){
    speedPerStep = 1; //just in case - prevents rounding from resulting in no accelleration at all.
  }
  unsigned int microDelay;
  if (synta.cmd.bVal(motor) < 160000){
    microDelay = rate >> 1; //number of uS to wait for initial speed (rate is in 1/2ths of a microsecond)
  } else {
    microDelay = rate >> 4; //number of uS to wait for initial speed (rate is in 1/16ths of a microsecond)
  }
  for(int i = 0;i < accellerateSteps;i++){
    delayMicroseconds(microDelay);
    if(synta.axis()){ //Step
      writeSTEP2(HIGH);
      delayMicroseconds(15);
      writeSTEP2(LOW);
    } else {
      writeSTEP1(HIGH);
      delayMicroseconds(15);
      writeSTEP1(LOW);
    }
    microDelay -= speedPerStep;
  }
  //Now at speed, so return, and timers will be used from now on.
}

//Timer Interrupt-----------------------------------------------------------------------------
void configureTimer(){
  TIMSK3 &= ~(1<<TOIE3); //disable timer so it can be configured
  TIMSK4 &= ~(1<<TOIE4); //disable timer so it can be configured
  //set to normal counting mode
  TCCR3A &= ~((1<<WGM31) | (1<<WGM30));
  TCCR3B &= ~((1<<WGM33) | (1<<WGM32));
  TCCR4A &= ~((1<<WGM41) | (1<<WGM40));
  TCCR4B &= ~((1<<WGM43) | (1<<WGM42));
  //Disable compare interrupt (only interested in overflow)
  TIMSK3 &= ~((1<<OCIE3A) | (1<<OCIE3B) | (1<<OCIE3C));
  TIMSK4 &= ~((1<<OCIE4A) | (1<<OCIE4B) | (1<<OCIE4C));
  if (synta.cmd.bVal(synta.axis()) < 160000){
    timerCountRate = 200000;
    //Set prescaler to F_CPU/8
    TCCR3B &= ~(1<<CS32); //0
    TCCR3B |= (1<<CS31);//1
    TCCR3B &= ~(1<<CS30);//0
    TCCR4B &= ~(1<<CS42); //0
    TCCR4B |= (1<<CS41);//1
    TCCR4B &= ~(1<<CS40);//0
  } else {
    timerCountRate = 1600000;
    //Set prescaler to F_CPU/1
    TCCR3B &= ~(1<<CS32); //0
    TCCR3B &= ~(1<<CS31);//0
    TCCR3B |= (1<<CS30);//1
    TCCR4B &= ~(1<<CS42); //0
    TCCR4B &= ~(1<<CS41);//0
    TCCR4B |= (1<<CS40);//1
  }
  Serial1.println((unsigned long)timerCountRate);
  unsigned long rate = timerCountRate * 650L; //min rate of just under the sidereal rate
  rate /= synta.cmd.bVal(synta.axis());
  rate *= 10; //corrects for timer count rate being set to 1/10th of the required to prevent numbers getting too big for an unsigned long.
  minTimerOVF = 65535L - rate;
}

/*Timer Interrupt Vector*/
ISR(TIMER3_OVF_vect) {
  TCNT3 = timerOVF[0]; //reset timer straight away to avoid compounding errors
  motorStep(0);
}

/*Timer Interrupt Vector*/
ISR(TIMER4_OVF_vect) {
  TCNT4 = timerOVF[1]; //reset timer straight away to avoid compounding errors
  motorStep(1);
}

void motorStep(byte motor){
  static char divider[2] = {0};
  if(motor){
    writeSTEP2(HIGH);
    delayMicroseconds(15);
    writeSTEP2(LOW);
  } else {
    writeSTEP1(HIGH);
    delayMicroseconds(15);
    writeSTEP1(LOW);
  }
  if(synta.cmd.stepDir(motor) < 0){
    divider[motor]--;
    if (divider[motor] < 0){
      divider[motor] = synta.scalar() - 1;
      synta.cmd.jVal(motor, (synta.cmd.jVal(motor) - 1));
      if(synta.cmd.gotoEn(motor)){
        long stepsLeft = gotoPosn[motor] - synta.cmd.jVal(motor);
        stepsLeft *= synta.cmd.stepDir(motor);
        if(stepsLeft <= (20/synta.scalar())){
          motorStop(motor); //will decellerate the rest of the way
        }
      }
    }
  } else {
    divider[motor]++;
    if (divider[motor] >= synta.scalar()){
      divider[motor] = 0;
      synta.cmd.jVal(motor, (synta.cmd.jVal(motor) + 1));
      if(synta.cmd.gotoEn(motor)){
        long stepsLeft = gotoPosn[motor] - synta.cmd.jVal(motor);
        stepsLeft *= synta.cmd.stepDir(motor);
        if(stepsLeft <= (20/synta.scalar())){
          motorStop(motor);
        }
      }
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
//--------------------------------------------------------------------------------------------

