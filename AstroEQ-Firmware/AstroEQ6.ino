/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Syntia
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts (Not EQ3-2, they have a different gear ratio)
 
  Current Verison: 6.2
*/

//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#include "synta.h" //Synta Communications Protocol.
#include "PinMappings.h" //Read Pin Mappings
#include "EEPROMReader.h" //Read config file
#include "EEPROMAddresses.h" //Config file addresses


//-------------------------------------------------------------
//DO NOT EDIT ANYTHING BELOW THIS LINE-------------------------
//-------------------------------------------------------------

Synta synta = Synta::getInstance(1281); //create a mount instance, specify version!

#define RA 0 //Right Ascension is AstroEQ axis 0 (Synta axis '1')
#define DC 1 //Declination is AstroEQ axis 1 (Synta axis '2')

#define DEBUG 2
unsigned int normalGotoSpeed[2];
unsigned int gotoFactor[2];
unsigned int minSpeed[2];
unsigned int stopSpeed[2];

unsigned long gotoPosn[2] = {0UL,0UL}; //where to slew to

#define timerCountRate 8000000

#define NormalDistnWidth 32
#define HighspeedDistnWidth 4
unsigned int timerOVF[2][max(HighspeedDistnWidth,NormalDistnWidth)];
boolean highSpeed[2] = {false,false};
byte distributionWidth[2] = {NormalDistnWidth,NormalDistnWidth};
#define distributionSegment(m) (m ? GPIOR1 : GPIOR0) 
#define decelerationSteps(m) (m ? DDRC : PORTC)
#define currentMotorSpeed(m) (m ? OCR3A : OCR3B)
#define interruptCount(m) (m ? OCR1A : OCR1B)
#define interruptOVFCount(m) (m ? ICR3 : ICR1)
#define interruptControlRegister(m) (m ? TIMSK3 : TIMSK1)
#define interruptControlBitMask(m) (m ? _BV(ICIE3) : _BV(ICIE1))
#define timerCountRegister(m) (m ? TCNT3 : TCNT1)
#define timerPrescalarRegister(m) (m ? TCCR3B : TCCR1B)

//Pins
const byte statusPin = statusPin_Define;
const byte resetPin[2] = {resetPin_0_Define,resetPin_1_Define};
const byte dirPin[2] = {dirPin_0_Define,dirPin_1_Define};
const byte enablePin[2] = {enablePin_0_Define,enablePin_1_Define};
const byte stepPin[2] = {stepPin_0_Define,stepPin_1_Define};
const byte st4Pin[2][2] = {{ST4AddPin_0_Define,ST4SubPin_0_Define},{ST4AddPin_1_Define,ST4SubPin_1_Define}};

#ifdef LEGACY_MODE
const byte modePins[2][2] = {{modePins0_0_Define,modePins2_0_Define},{modePins0_1_Define,modePins2_1_Define}};
#else
const byte modePins[2][3] = {{modePins0_0_Define,modePins1_0_Define,modePins2_0_Define},{modePins0_1_Define,modePins1_1_Define,modePins2_1_Define}};
#endif
 

byte encodeDirection[2];

#define _STEP0_HIGH _BV(digitalPinToBit(stepPin[RA]))
#define _STEP0_LOW (~_STEP0_HIGH)
#define STEP0PORT *digitalPinToPortReg(stepPin[RA])
#define _STEP1_HIGH _BV(digitalPinToBit(stepPin[DC]))
#define _STEP1_LOW (~_STEP1_HIGH)
#define STEP1PORT *digitalPinToPortReg(stepPin[DC])
#define stepPort(m) (m ? STEP1PORT : STEP0PORT)
#define stepHigh(m) (m ? _STEP1_HIGH : _STEP0_HIGH)
#define stepLow(m) (m ? _STEP1_LOW : _STEP0_LOW)

#ifdef LEGACY_MODE
byte modeState[2][2] = {{HIGH,HIGH},{LOW,LOW}};
#define MODE0 0
#define MODE2 1
#define STATE16 0
#define STATE2 1

#else
byte modeState[2][3] = {{HIGH,HIGH,HIGH},{HIGH,LOW,LOW}};
#define MODE0 0
#define MODE1 1
#define MODE2 2
#define STATE16 0
#define STATE2 1

#endif

boolean checkEEPROM(){
  char temp[9] = {0};
  EEPROM.readString(temp,8,AstroEQID_Address);
  return !(strncmp(temp,"AstroEQ",8));
}

void systemInitialiser(){
  encodeDirection[RA] = EEPROM.readByte(RAReverse_Address);  //reverse the right ascension if 1
  encodeDirection[DC] = EEPROM.readByte(DECReverse_Address); //reverse the declination if 1
  

#ifdef LEGACY_MODE
  modeState[STATE16][MODE0] = !EEPROM.readByte(Driver_Address);
  modeState[STATE2][MODE0] =  !modeState[STATE16][MODE0];
#else
  modeState[STATE16][MODE0] = !EEPROM.readByte(Driver_Address);
  modeState[STATE16][MODE1] = modeState[STATE16][MODE0];
#endif
  
  gotoFactor[RA] = EEPROM.readByte(RAGoto_Address);
  normalGotoSpeed[RA] = synta.cmd.stepIncrement[RA] * gotoFactor[RA] + 1;
  gotoFactor[DC] = EEPROM.readByte(DECGoto_Address);
  normalGotoSpeed[DC] = synta.cmd.stepIncrement[DC] * gotoFactor[DC] + 1;
  
  minSpeed[RA] = synta.cmd.siderealIVal[RA] + ((unsigned int)synta.cmd.stepIncrement[RA] << 2);
  minSpeed[DC] = synta.cmd.siderealIVal[DC] + ((unsigned int)synta.cmd.stepIncrement[DC] << 2);
  
  highSpeed[RA] = !EEPROM.readByte(Microstep_Address);
  highSpeed[DC] = highSpeed[RA];
  distributionWidth[RA] = highSpeed[RA] ? HighspeedDistnWidth : NormalDistnWidth;
  distributionWidth[DC] = highSpeed[DC] ? HighspeedDistnWidth : NormalDistnWidth;
  
#ifdef DEBUG
  Serial1.println();
  Serial1.println(minSpeed[RA]);
  Serial1.println(minSpeed[DC]);
  Serial1.println();
  Serial1.println(normalGotoSpeed[RA]);
  Serial1.println(normalGotoSpeed[DC]);
  Serial1.println();
  Serial1.println(synta.cmd.stepIncrement[RA]);
  Serial1.println(synta.cmd.stepIncrement[DC]);
  Serial1.println();
#endif
  if(!checkEEPROM()){
    while(1); //prevent AstroEQ startup if EEPROM is blank.
  }
}

void setup()
{
#ifdef DEBUG
  Serial1.begin(115200);
#endif
  systemInitialiser();
  
#if defined(__AVR_ATmega162__)
  //Timer 2 registers are being used as general purpose data storage for high efficency interrupt routines. So timer must be fully disabled.
  TCCR2 = 0;
  ASSR=0;
#endif
  
  pinMode(statusPin,OUTPUT);
  for(byte i = 0;i < 2;i++){ //for each motor...
    pinMode(enablePin[i],OUTPUT); //enable the Enable pin
    digitalWrite(enablePin[i],HIGH); //IC disabled
    pinMode(stepPin[i],OUTPUT); //enable the step pin
    digitalWrite(stepPin[i],LOW); //default is low
    pinMode(dirPin[i],OUTPUT); //enable the direction pin
    digitalWrite(dirPin[i],LOW); //default is low
    const byte* modePin = modePins[i];
    const byte* state = modeState[highSpeed[i]];
#ifdef LEGACY_MODE
    for(byte j = 0; j < 2; j++){
      pinMode(modePin[j],OUTPUT);
      digitalWrite(modePin[j],state[j]);
    }
//    pinMode(modePins[i][0],OUTPUT); //enable the mode pins
//    pinMode(modePins[i][1],OUTPUT); //enable the mode pins
//    digitalWrite(modePins[i][0],modeState[highSpeed[i]][0]);
//    digitalWrite(modePins[i][1],modeState[highSpeed[i]][1]);
#else
    for(byte j = 0; j < 3; j++){
      pinMode(modePin[j],OUTPUT);
      digitalWrite(modePin[j],state[j]);
//      pinMode(modePins[i][0],OUTPUT); //enable the mode pins
//      pinMode(modePins[i][1],OUTPUT); //enable the mode pins
//      pinMode(modePins[i][2],OUTPUT); //enable the mode pins
//      digitalWrite(modePins[i][0],modeState[highSpeed[i]][0]);
//      digitalWrite(modePins[i][1],modeState[highSpeed[i]][1]);
//      digitalWrite(modePins[i][2],modeState[highSpeed[i]][2]);
    }
#endif
    pinMode(resetPin[i],OUTPUT); //enable the reset pin
    digitalWrite(resetPin[i],LOW); //active low reset
    delay(1); //allow ic to reset
    digitalWrite(resetPin[i],HIGH); //complete reset
    
    pinMode(st4Pin[i][0],INPUT_PULLUP);
    pinMode(st4Pin[i][1],INPUT_PULLUP);
  }
    
  // start Serial port:
  Serial.begin(9600); //SyncScan runs at 9600Baud, use a serial port of your choice 
  while(Serial.available()){
    Serial.read(); //Clear the buffer
  }
  
  configureTimer(); //setup the motor pulse timers.
  
  //setup interrupt for ST4 connection
#if defined(__AVR_ATmega162__)
  PCMSK0 = 0xF0; //PCINT[4..7]
#define PCICR GICR
#else
  PCMSK0 = 0x0F; //PCINT[0..3]
  PCICR &= ~_BV(PCIE2); //disable PCInt[16..23] vector
#endif
  PCICR &= ~_BV(PCIE1); //disable PCInt[8..15] vector
  PCICR |= _BV(PCIE0); //enable PCInt[0..7] vector
}

void loop(){
  static unsigned long lastMillis = millis();
  static boolean isLedOn = false;
  char decodedPacket[11]; //temporary store for completed command ready to be processed
  if (Serial.available()) { //is there a byte in buffer
    digitalWrite(statusPin,HIGH); //Turn on the LED to indicate activity.
    char recievedChar = Serial.read(); //get the next character in buffer
#ifdef DEBUG
    Serial1.write(recievedChar);
#endif
    char decoded = synta.recieveCommand(decodedPacket,recievedChar); //once full command packet recieved, decodedPacket returns either error packet, or valid packet
    if (decoded == 1){ //Valid Packet
      decodeCommand(synta.command(),decodedPacket); //decode the valid packet
    } else if (decoded == -1){ //Error
      Serial.print(decodedPacket); //send the error packet (recieveCommand() already generated the error packet, so just need to send it)
#ifdef DEBUG
      Serial1.print(F(" ->Res:"));
      Serial1.println(decodedPacket);
#endif
    } //otherwise command not yet fully recieved, so wait for next byte
    lastMillis = millis();
    isLedOn = true;
  }
  if (isLedOn && (millis() > lastMillis + 20)){
    isLedOn = false;
    digitalWrite(statusPin,LOW); //Turn LED back off after a short delay.
  }
#ifdef DEBUG
  if (Serial1.available()) { //is there a byte in buffer
    Serial1.println();
    Serial1.println(currentMotorSpeed(RA));
    Serial1.println(currentMotorSpeed(DC));
    Serial1.println(interruptOVFCount(RA));
    Serial1.println(interruptOVFCount(DC));
    Serial1.println();
    Serial1.read();
  }
#endif
}

void decodeCommand(char command, char* packetIn){ //each command is axis specific. The axis being modified can be retrieved by calling synta.axis()
  char response[11]; //generated response string
  unsigned long responseData = 0; //data for response
  byte axis = synta.axis();
  unsigned int correction;
  //byte scalar = synta.scalar[axis]+1;
  uint8_t oldSREG;
  switch(command){
    case 'e': //readonly, return the eVal (version number)
        responseData = synta.cmd.eVal[axis]; //response to the e command is stored in the eVal function for that axis.
        break;
    case 'a': //readonly, return the aVal (steps per axis)
        responseData = synta.cmd.aVal[axis]; //response to the a command is stored in the aVal function for that axis.
        break;
    case 'b': //readonly, return the bVal (sidereal step rate)
        responseData = synta.cmd.bVal[axis]; //response to the b command is stored in the bVal function for that axis.
        correction = (synta.cmd.siderealIVal[axis] << 1);
        responseData = (responseData * (correction+1))/correction; //account for rounding inside Skywatcher DLL.
        break;
    case 'g': //readonly, return the gVal (high speed multiplier)
        responseData = synta.cmd.gVal[axis]; //response to the g command is stored in the gVal function for that axis.
        break;
    case 's': //readonly, return the sVal (steps per worm rotation)
        responseData = synta.cmd.sVal[axis]; //response to the s command is stored in the sVal function for that axis.
        break;
    case 'f': //readonly, return the fVal (axis status)
        responseData = synta.cmd.fVal(axis); //response to the f command is stored in the fVal function for that axis.
        break;
    case 'j': //readonly, return the jVal (current position)
        oldSREG = SREG; 
        cli();  //The next bit needs to be atomic, just in case the motors are running
        responseData = synta.cmd.jVal[axis]; //response to the j command is stored in the jVal function for that axis.
        SREG = oldSREG;
        break;
    case 'K': //stop the motor, return empty response
        motorStop(axis,0); //normal ISR based decelleration trigger.
        break;
    case 'L':
        motorStop(axis,1); //emergency axis stop.
        motorDisable(axis); //shutdown driver power.
        break;
    case 'G': //set mode and direction, return empty response
        if (packetIn[0] == '0'){
          packetIn[0] = '2'; //don't allow a high torque goto. But do allow a high torque slew.
        }
        synta.cmd.setGVal(axis, (packetIn[0] - '0')); //Store the current mode for the axis
        synta.cmd.setDir(axis, (packetIn[1] - '0')); //Store the current direction for that axis
        break;
    case 'H': //set goto position, return empty response (this sets the number of steps to move from cuurent position if in goto mode)
        synta.cmd.setHVal(axis, synta.hexToLong(packetIn)); //set the goto position container (convert string to long first)
        break;
    case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
        responseData = synta.hexToLong(packetIn);
        //if (responseData > minSpeed[axis]){
        //  responseData = minSpeed[axis]; //minimum speed limit <- if it is slower than this, there will be no acelleration.
        //}
        synta.cmd.setIVal(axis, responseData); //set the speed container (convert string to long first) 
        responseData = 0;
        break;
    case 'E': //set the current position, return empty response
        oldSREG = SREG; 
        cli();  //The next bit needs to be atomic, just in case the motors are running
        synta.cmd.setjVal(axis, synta.hexToLong(packetIn)); //set the current position (used to sync to what EQMOD thinks is the current position at startup
        SREG = oldSREG;
        break;
    case 'F': //Enable the motor driver, return empty response
        motorEnable(axis); //This enables the motors - gives the motor driver board power
        break;
    default: //Return empty response (deals with commands that don't do anything before the response sent (i.e 'J'), or do nothing at all (e.g. 'M', 'L') )
        break;
  }
  
  synta.assembleResponse(response, command, responseData); //generate correct response (this is required as is)
  Serial.print(response); //send response to the serial port
#ifdef DEBUG

#if DEBUG == 2
  if (command != 'j')
#endif
  {
    Serial1.print(F(" ->Res:"));
    Serial1.println(response);
  }
#endif
  
  if(command == 'J'){ //J tells
    if(synta.cmd.GVal[axis] & 1){
      
      //This is the funtion that enables a slew type move.
      slewMode(axis); //Slew type
      
      
    } else {
      
      //This is the function for goto mode. You may need to customise it for a different motor driver
      gotoMode(axis); //Goto Mode
      
      
    }
  }
}



//Calculates the rate based on the recieved I value 
void calculateRate(byte motor){
  
  unsigned long rate;
  unsigned long remainder;
  float floatRemainder;
  unsigned long divisor = synta.cmd.bVal[motor];
 // byte GVal = synta.cmd.GVal[motor];
  //if((GVal == 2)||(GVal == 1)){ //Normal Speed}
  if(!highSpeed[motor]){ //Normal Speed
  //  highSpeed[motor] = false;
    distributionWidth[motor] = NormalDistnWidth;
  } else { //High Speed
  //  highSpeed[motor] = true;
    distributionWidth[motor] = HighspeedDistnWidth; //There are 8 times fewer steps over which to distribute extra clock cycles
  }
  
  //When dividing a very large number by a much smaller on, float accuracy is abismal. So firstly we use integer math to split the division into quotient and remainder
  rate = timerCountRate / divisor; //Calculate the quotient
  remainder = timerCountRate % divisor; //Calculate the remainder
  
  //Then convert the remainder into a decimal number (division of a small number by a larger one, improving accuracy)
  floatRemainder = (float)remainder/(float)divisor; //Convert the remainder to a decimal.
    
  //Multiply the remainder by distributionWidth to work out an approximate number of extra clocks needed per full step (each step is 'distributionWidth' microsteps)
  floatRemainder *= (float)distributionWidth[motor]; 
  //This many extra cycles are needed:
  remainder = (unsigned long)round(floatRemainder); 
  
  //Now truncate to an unsigned int with a sensible max value (the int is to avoid register issues with the 16 bit timer)
  if(rate > 65535UL){
    rate = 65535UL;
  } else if (rate < 128UL) {
    rate = 128UL;
  }
#if defined(__AVR_ATmega162__)
  rate--;
#endif
  
  for (byte i = 0; i < distributionWidth[motor]; i++){
#if defined(__AVR_ATmega162__)
    timerOVF[motor][i] = rate; //Subtract 1 as timer is 0 indexed.
#else
    timerOVF[motor][i] = rate; //Hmm, for some reason this one doesn't need 1 subtracting???
#endif
  }
  
  //evenly distribute the required number of extra clocks over the full step.
  for (unsigned long i = 0; i < remainder; i++){
    float distn = i;
    distn *= (float)distributionWidth[motor];
    distn /= (float)remainder;
    byte index = (byte)ceil(distn);
    timerOVF[motor][index] += 1;
  }
  
#ifdef DEBUG
  Serial1.println();
  Serial1.println(rate);
  Serial1.println();
#endif
  
  distributionWidth[motor]--; //decrement to get its bitmask. (distnWidth is always a power of 2)
}

void motorEnable(byte axis){
  digitalWrite(enablePin[axis],LOW);
  synta.cmd.setFVal(axis,1);
  calculateRate(axis); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
}

void motorDisable(byte axis){
  digitalWrite(enablePin[axis],LOW);
  synta.cmd.setFVal(axis,0);
}

void slewMode(byte axis){
  motorStart(axis,1/*decellerateStepsFactor[axis]*/); //Begin PWM
}

void gotoMode(byte axis){
  if (synta.cmd.HVal[axis] < 2){
    synta.cmd.setHVal(axis,2);
  }
  unsigned long HVal = synta.cmd.HVal[axis];
  unsigned long halfHVal = HVal >> 1;
  unsigned int gotoSpeed = normalGotoSpeed[axis];
  byte decelerationLength;
  if (halfHVal < 80) {
    decelerationLength = (byte)halfHVal;
    if (gotoFactor[axis] < 4) {
      gotoSpeed += synta.cmd.stepIncrement[axis];
    }
    if (gotoFactor[axis] < 8) {
      gotoSpeed += synta.cmd.stepIncrement[axis]; //for short goto's when goto speed is very high, use a slower target speed.
    }
  } else {
    decelerationLength = 80;
  }
  //byte decelerationLength = (byte)min(halfHVal,80);
#ifdef DEBUG
  Serial1.println();
  Serial1.println(decelerationLength);
  Serial1.println(synta.cmd.jVal[axis]);
#endif
  gotoPosn[axis] = synta.cmd.jVal[axis] + (synta.cmd.stepDir[axis] * (synta.cmd.HVal[axis] - (unsigned long)decelerationLength)); //current position + relative change - decelleration region
    
#ifdef DEBUG
  Serial1.println(gotoPosn[axis]);
  Serial1.println();
#endif
  synta.cmd.setIVal(axis, gotoSpeed);
  //calculateRate(axis);
  motorStart(axis,decelerationLength/*decellerateStepsFactor[axis]*/); //Begin PWM
  synta.cmd.setGotoEn(axis,1);
}

void timerEnable(byte motor, byte mode) {
  //if (mode){
    //FCPU/8
  //  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn0)); //0x0
  //  timerPrescalarRegister(motor) |= (1<<CSn1);//x1x
  //} else {
    //FCPU/1
  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn1));//00x
  timerPrescalarRegister(motor) |= (1<<CSn0);//xx1
  //}
  synta.cmd.setStepLength(motor, 1);//(mode ? synta.cmd.gVal[motor] : 1));
/*#ifdef LEGACY_MODE
  for( byte j = 0; j < 2; j++){
    digitalWrite(modePins[motor][j],modeState[mode][j]);
  }
#else
  for( byte j = 0; j < 3; j++){
    digitalWrite(modePins[motor][j],modeState[mode][j]);
  }
#endif*/
}

inline void timerDisable(byte motor) {
  interruptControlRegister(motor) &= ~interruptControlBitMask(motor); //Disable timer interrupt
  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn1) | (1<<CSn0));//00x
}

void motorStart(byte motor, byte gotoDeceleration){
  digitalWrite(dirPin[motor],encodeDirection[motor]^synta.cmd.dir[motor]); //set the direction
  synta.cmd.setStopped(motor, 0);
  
#ifdef DEBUG
  Serial1.println(F("IVal:"));
  Serial1.println(synta.cmd.IVal[motor]);
  Serial1.println();
#endif
  
  unsigned int startSpeed = minSpeed[motor];
  if (synta.cmd.IVal[motor] > minSpeed[motor]){
    startSpeed = synta.cmd.IVal[motor]; //This is only the case with ST4 on the DEC axis.
  } else if ((startSpeed > 650) && (synta.cmd.IVal[motor] <= 650)){
    startSpeed = 650; //if possible start closer to the target speed to avoid *very* long accelleration times. 
  }
  stopSpeed[motor] = startSpeed;
#ifdef DEBUG
  Serial1.println(F("StartSpeed:"));
  Serial1.println(startSpeed);
  Serial1.println();
#endif
  
  interruptCount(motor) = 1;
  currentMotorSpeed(motor) = startSpeed;//minSpeed[motor];
  distributionSegment(motor) = 0;
  decelerationSteps(motor) = -gotoDeceleration;
  timerCountRegister(motor) = 0;
  interruptControlRegister(motor) |= interruptControlBitMask(motor);
  interruptOVFCount(motor) = timerOVF[motor][0];
  timerEnable(motor,highSpeed[motor]);
}

void motorStop(byte motor, byte emergency){
  if (emergency) {
    //trigger instant shutdown of the motor in an emergency.
    synta.cmd.setGotoEn(motor,0); //Not in goto mode.
    synta.cmd.setStopped(motor,1); //mark as stopped
    timerDisable(motor);
    
  } else if (!synta.cmd.stopped[motor]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
    //trigger ISR based decelleration
    synta.cmd.IVal[motor] = stopSpeed[motor] + synta.cmd.stepIncrement[motor]; 
  }
}

//Timer Interrupt-----------------------------------------------------------------------------
void configureTimer(){
  interruptControlRegister(DC) = 0; //disable all timer interrupts.
#if defined(__AVR_ATmega162__)
  interruptControlRegister(RA) &= 0b00000011; //for 162, the lower 2 bits of the declination register control another timer, so leave them alone.
#else
  interruptControlRegister(RA) = 0;
#endif
  //set to ctc mode (0100)
  TCCR1A = 0;//~((1<<WGM11) | (1<<WGM10));
  TCCR1B = ((1<<WGM12) | (1<<WGM13));
  TCCR3A = 0;//~((1<<WGM31) | (1<<WGM30));
  TCCR3B = ((1<<WGM32) | (1<<WGM33));
}


unsigned int divu3b(unsigned int n) { //accurate for  n < 0x7FFF
  unsigned long b = n+1;
  n = ((b * 0x5555) >> 16);
  return n;
}
unsigned int divu5b(unsigned int n) { //accurate for n < 0x7FFF
  unsigned long b = n+1;
  n = ((b * 0x3333) >> 16);
  return n = b >> 16;
}

ISR(PCINT0_vect) {
  //ST4 Pin Change Interrupt Handler.
  byte stopped;
  if(!synta.cmd.gotoEn[RA]){
    //Only allow when not it goto mode.
    stopped = synta.cmd.stopped[RA];
    unsigned int newSpeed = synta.cmd.siderealIVal[RA];
    if (synta.cmd.dir[RA] && !stopped) {
      goto ignoreRAST4; //if travelling in the wrong direction, then ignore.
    }
    byte multiplier;
    if (!(*digitalPinToPinReg(st4Pin[RA][1]) & _BV(digitalPinToBit(st4Pin[RA][1])))) {
      //RA-
      multiplier = 0x55; //------------ 0.75x sidereal rate
      goto setRASpeed;
    } else if (!(*digitalPinToPinReg(st4Pin[RA][0]) & _BV(digitalPinToBit(st4Pin[RA][0])))) {
      //RA+
      multiplier = 0x33; //------------ 1.25x sidereal rate
setRASpeed:
      unsigned long working = multiplier;
      newSpeed = newSpeed << 2;
      asm volatile( //This is a division by 5, or by 3. Which depends on multiplier (0x55 = /3, 0x33 = /5)
        "subi r18, 0xFF \n\t"  //n++
        "sbci r19, 0xFF \n\t"
        "mul  %A0, %A2  \n\t"  //A * X
        "mov  %B2,  r1  \n\t"
        
        "add  %B2,  r0  \n\t"  //A* X'
        "adc  %C2,  r1  \n\t"
        
        "mul  %B0, %A1  \n\t"
        "add  %B2,  r0  \n\t"  //A* X'
        "adc  %C2,  r1  \n\t"
        "adc  %D2, %D2  \n\t"  //D0 is known 0, we need to grab the carry
        
        "add  %C2,  r0  \n\t"  //A* X'
        "adc  %D2,  r1  \n\t"
        
        "eor   r1,  r1  \n\t"
        
        "movw %A0, %C2  \n\t"  // >> 16
        
        : "=r" (newSpeed)
        : "0" (newSpeed), "r" (working)
        : "r1", "r0"
      );
      synta.cmd.IVal[RA] = newSpeed;
      if (stopped) {
        synta.cmd.dir[RA] = 0; //set direction
        register byte one asm("r22");
        asm volatile(
          "ldi %0,0x01  \n\t"
          : "=r" (one)
          : "0" (one)
          :
        );
        synta.cmd.stepDir[RA] = one; //set step direction
        synta.cmd.GVal[RA] = one; //slew mode
        //calculateRate(RA);
        motorStart(RA,one);
      } else {
        if (stopSpeed[RA] < synta.cmd.IVal[RA]) {
          stopSpeed[RA] = synta.cmd.IVal[RA]; //ensure that RA doesn't stop.
        }
      }
    } else {
ignoreRAST4:
      synta.cmd.IVal[RA] = newSpeed;
    }
  }
  if(!synta.cmd.gotoEn[DC]){
    //Only allow when not it goto mode.
    byte dir;
    byte stepDir;
    if (!(*digitalPinToPinReg(st4Pin[DC][1]) & _BV(digitalPinToBit(st4Pin[DC][1])))) {
      //DEC-
      dir = 1;
      stepDir = -1;
      goto setDECSpeed;
    } else if (!(*digitalPinToPinReg(st4Pin[DC][0]) & _BV(digitalPinToBit(st4Pin[DC][0])))) {
      //DEC+
      dir = 0;
      stepDir = 1;
setDECSpeed:
      synta.cmd.stepDir[DC] = stepDir; //set step direction
      synta.cmd.dir[DC] = dir; //set direction
      synta.cmd.IVal[DC] = (synta.cmd.siderealIVal[RA] << 2); //move at 0.25x sidereal rate
      synta.cmd.GVal[DC] = 1; //slew mode
      //calculateRate(DC);
      motorStart(DC,1);
    } else {
      synta.cmd.IVal[DC] = stopSpeed[DC] + synta.cmd.stepIncrement[DC];
    }
  }
}

/*Timer Interrupt Vector*/
ISR(TIMER3_CAPT_vect) {
  byte timeSegment = distributionSegment(DC);
  byte index = (distributionWidth[DC] << 1) & timeSegment;
  interruptOVFCount(DC) = *(int*)((byte*)timerOVF[DC] + index); //move to next pwm period
  distributionSegment(DC) = timeSegment + 1;
  
  unsigned int count = interruptCount(DC)-1; //OCR1B is used as it is an unused register which affords us quick access.
  if (count == 0) {
    register byte port asm("r18") = stepPort(DC);
    register unsigned int startVal asm("r24") = currentMotorSpeed(DC);
    //byte port = STEP0PORT;
    //unsigned int startVal = currentMotorSpeed(DC);
    interruptCount(DC) = startVal; //start val is used for specifying how many interrupts between steps. This tends to IVal after acceleration
    if (port & stepHigh(DC)){
      stepPort(DC) = port & stepLow(DC);
      unsigned long jVal = synta.cmd.jVal[DC];
      jVal = jVal + synta.cmd.stepDir[DC];
      synta.cmd.jVal[DC] = jVal;
      if(synta.cmd.gotoEn[DC]){
        if (gotoPosn[DC] == jVal){ //reached the decelleration marker. Note that this line loads gotoPosn[] into r24:r27
          //will decellerate the rest of the way. So first move gotoPosn[] to end point.
          if (decelerationSteps(DC) & _BV(7)) {
            /*
            unsigned long gotoPos = gotoPosn[DC];
            if (synta.cmd.stepDir[DC] < 0){
              gotoPosn[DC] = gotoPos + decelerationSteps(DC);
            } else {
              gotoPosn[DC] = gotoPos - decelerationSteps(DC);
            }
            */
            //--- This is a heavily optimised version of the code commented out just above ------------
            //During compare of jVal and gotoPosn[], gotoPosn[] was loaded into r24 to r27
            register char stepDir asm("r19") = synta.cmd.stepDir[DC];
            asm volatile(
              "in   %A0, %2  \n\t"
              "cp   %1, __zero_reg__  \n\t"
              "brlt .+4      \n\t"
              "neg  %A0      \n\t"
              "eor  %B0, %B0   \n\t"
              "mov  %C0, %B0   \n\t"
              "mov  %D0, %B0   \n\t"
              "add  %A0, r24  \n\t" //add previously loaded gotoPosn[] registers to new gotoPosn[] registers.
              "adc  %B0, r25  \n\t"
              "adc  %C0, r26  \n\t"
              "adc  %D0, r27  \n\t"
              : "=a" (gotoPosn[DC]) //goto selects r18:r21. This adds sts commands for all 4 bytes
              : "r" (stepDir),"I" (_SFR_IO_ADDR(decelerationSteps(DC)))       //stepDir is in r19 to match second byte of goto.
              :
            );
            //-----------------------------------------------------------------------------------------
            decelerationSteps(DC) = 0; //say we are stopping
            synta.cmd.IVal[DC] = stopSpeed[DC]; //decellerate to min speed. This is possible in at most 80 steps.
          } else {
            goto stopMotorISR3;
          }
        }
      } 
      if (currentMotorSpeed(DC) > stopSpeed[DC]) {
stopMotorISR3:
        synta.cmd.gotoEn[DC] = 0; //Not in goto mode.
        synta.cmd.stopped[DC] = 1; //mark as stopped
        timerDisable(DC);
      }
    } else {
      stepPort(DC) = port | stepHigh(DC);
      register unsigned int iVal asm("r20") = synta.cmd.IVal[DC];
      //unsigned int iVal = synta.cmd.IVal[RA];
      register byte increment asm("r0") = synta.cmd.stepIncrement[DC];
      asm volatile(
        "movw r18, %0   \n\t"
        "sub  %A0, %2    \n\t"
        "sbc  %B0, __zero_reg__    \n\t"
        "cp   %A0, %A1   \n\t"
        "cpc  %B0, %B1   \n\t"
        "brge 1f         \n\t" //branch if iVal <= (startVal-increment)
        "movw  %0, r18   \n\t"
        "add  %A0, %2    \n\t"
        "adc  %B0, __zero_reg__    \n\t"
        "cp   %A1, %A0   \n\t"
        "cpc  %B1, %B0   \n\t"
        "brge 1f         \n\t" //branch if (startVal+increment) <= iVal
        "movw  %0, %1   \n\t"  //copy iVal to startVal
        "1:             \n\t"
        : "=&w" (startVal) //startVal is in r24:25
        : "a" (iVal), "t" (increment) //iVal is in r20:21
        : 
      );
      currentMotorSpeed(DC) = startVal; //store startVal
      /*
      if (startVal - increment >= iVal) { // if (iVal <= startVal-increment)
        startVal = startVal - increment;
      } else if (startVal + increment <= iVal){
        startVal = startVal + increment;
      } else {
        startVal = iVal;
      }
      currentMotorSpeed(DC) = startVal;
      */
    }
  } else {
    interruptCount(DC) = count;
  }
}

/*Timer Interrupt Vector*/
ISR(TIMER1_CAPT_vect) {
  byte timeSegment = distributionSegment(RA);
  byte index = (distributionWidth[RA] << 1) & timeSegment;
  
#if defined(__AVR_ATmega162__)
  asm("nop"); //ICR1 is two clocks faster to write to than ICR3, so account for that 
  asm("nop");
#endif
  interruptOVFCount(RA) = *(int*)((byte*)timerOVF[RA] + index); //move to next pwm period
  distributionSegment(RA) = timeSegment + 1;
  
  unsigned int count = interruptCount(RA)-1;
  if (count == 0) {
    register byte port asm("r18") = stepPort(RA);
    register unsigned int startVal asm("r24") = currentMotorSpeed(RA);
    //byte port = STEP1PORT;
    //unsigned int startVal = currentMotorSpeed(RA);
    interruptCount(RA) = startVal; //start val is used for specifying how many interrupts between steps. This tends to IVal after acceleration
    if (port & stepHigh(RA)){
      stepPort(RA) = port & stepLow(RA);
      unsigned long jVal = synta.cmd.jVal[RA];
      jVal = jVal + synta.cmd.stepDir[RA];
      synta.cmd.jVal[RA] = jVal;
      if(synta.cmd.gotoEn[RA]){
        if (jVal == gotoPosn[RA]){ //reached the decelleration marker
          //will decellerate the rest of the way. So first move gotoPosn[] to end point.
          if (decelerationSteps(RA) & _BV(7)) {
            /*
            unsigned long gotoPos = gotoPosn[RA];
            if (synta.cmd.stepDir[RA] < 0){
              gotoPosn[RA] = gotoPos + decelerationSteps(RA);
            } else {
              gotoPosn[RA] = gotoPos - decelerationSteps(RA);
            }
            */
            //--- This is a heavily optimised version of the code commented out just above ------------
            register char temp asm("r19") = synta.cmd.stepDir[RA];
            asm volatile(
              "in   %A0, %2  \n\t"
              "cp   %1, __zero_reg__  \n\t"
              "brlt .+4      \n\t"
              "neg  %A0      \n\t"
              "eor  %B0, %B0   \n\t"
              "mov  %C0, %B0   \n\t"
              "mov  %D0, %B0   \n\t"
              "add  %A0, r24  \n\t"
              "adc  %B0, r25  \n\t"
              "adc  %C0, r26  \n\t"
              "adc  %D0, r27  \n\t"
              : "=a" (gotoPosn[RA]) //goto selects r18:r21. This adds sts commands for all 4 bytes
              : "r" (temp),"I" (_SFR_IO_ADDR(decelerationSteps(RA)))       //temp is in r19 to match second byte of goto.
              :
            );
            decelerationSteps(RA) = 0; //say we are stopping
            synta.cmd.IVal[RA] = stopSpeed[RA]; //decellerate to min speed. This is possible in at most 80 steps.
          } else {
            goto stopMotorISR1;
          }
        }
      } 
      if (currentMotorSpeed(RA) > stopSpeed[RA]) {
stopMotorISR1:
        //interruptControlRegister(RA) &= ~interruptControlBitMask(RA);
        synta.cmd.gotoEn[RA] = 0; //Not in goto mode.
        synta.cmd.stopped[RA] = 1; //mark as stopped
        timerDisable(RA);
      }
    } else {
      stepPort(RA) = port | stepHigh(RA);
      register unsigned int iVal asm("r20") = synta.cmd.IVal[RA];
      //unsigned int iVal = synta.cmd.IVal[RA];
      register byte increment asm("r0") = synta.cmd.stepIncrement[RA];
      asm volatile(
        "movw r18, %0   \n\t"
        "sub  %A0, %2    \n\t"
        "sbc  %B0, __zero_reg__    \n\t"
        "cp   %A0, %A1   \n\t"
        "cpc  %B0, %B1   \n\t"
        "brge 1f         \n\t" //branch if iVal <= (startVal-increment)
        "movw  %0, r18   \n\t"
        "add  %A0, %2    \n\t"
        "adc  %B0, __zero_reg__    \n\t"
        "cp   %A1, %A0   \n\t"
        "cpc  %B1, %B0   \n\t"
        "brge 1f         \n\t" //branch if (startVal+increment) <= iVal
        "movw  %0, %1   \n\t"  //copy iVal to startVal
        "1:             \n\t"
        : "=&w" (startVal) //startVal is in r24:25
        : "a" (iVal), "t" (increment) //iVal is in r20:21
        : 
      );
      currentMotorSpeed(RA) = startVal; //store startVal
      /*
      if (startVal - increment >= iVal) { // if (iVal <= startVal-increment)
        startVal = startVal - increment;
      } else if (startVal + increment <= iVal){
        startVal = startVal + increment;
      } else {
        startVal = iVal;
      }
      currentMotorSpeed(RA) = startVal;
      */
    }
  } else {
    interruptCount(RA) = count;
  }
}

#else
#error Unsupported Part! Please use an Arduino Mega, or ATMega162
#endif

