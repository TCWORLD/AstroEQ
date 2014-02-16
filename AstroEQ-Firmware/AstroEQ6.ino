/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Syntia
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts
 
  Current Verison: 7.0
*/

//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#include "synta.h" //Synta Communications Protocol.
#include "PinMappings.h" //Read Pin Mappings
#include "EEPROMReader.h" //Read config file
#include "EEPROMAddresses.h" //Config file addresses
#include <util/delay.h>

Synta synta = Synta::getInstance(1281); //create a mount instance, specify version!

#define RA 0 //Right Ascension is AstroEQ axis 0 (Synta axis '1')
#define DC 1 //Declination is AstroEQ axis 1 (Synta axis '2')

//#define DEBUG 1
byte stepIncrement[2];
unsigned int normalGotoSpeed[2];
unsigned int gotoFactor[2];
unsigned int minSpeed[2];
unsigned int stopSpeed[2];
byte readyToGo[2] = {0,0};
volatile byte gotoRunning[2] = {0,0};
unsigned long gotoPosn[2] = {0UL,0UL}; //where to slew to

#define timerCountRate 8000000

#define NormalDistnWidth 32
#define HighspeedDistnWidth 4
unsigned int timerOVF[2][max(HighspeedDistnWidth,NormalDistnWidth)];
boolean highSpeed[2] = {false,false};
byte distributionWidth[2] = {NormalDistnWidth,NormalDistnWidth};
byte stepIncrementRepeat[2] = {0,0};
unsigned int gotoDecelerationLength[2];
#define distributionSegment(m) (m ? GPIOR1 : GPIOR0) 
#define decelerationStepsLow(m) (m ? EEDR : DDRC)
#define decelerationStepsHigh(m) (m ? EEARL : PORTC)
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
  //modeState[STATE2][MODE0] =  EEPROM.readByte(Driver_Address);
  //modeState[STATE16][MODE0] = !modeState[STATE2][MODE0];
  modeState[STATE2][MODE0] =  EEPROM.readByte(Driver_Address);
#else
  //modeState[STATE16][MODE1] = !EEPROM.readByte(Driver_Address);
  //modeState[STATE16][MODE0] = modeState[STATE16][MODE1];
  modeState[STATE16][MODE0] = !EEPROM.readByte(Driver_Address);
  modeState[STATE2][MODE0] = modeState[STATE16][MODE0];
  modeState[STATE2][MODE1] = !modeState[STATE16][MODE0];
#endif
  
  unsigned int multiplierRA = synta.cmd.siderealIVal[RA]/75;
  unsigned int multiplierDC = synta.cmd.siderealIVal[DC]/75;
  gotoFactor[RA] = EEPROM.readByte(RAGoto_Address);
  normalGotoSpeed[RA] = multiplierRA * gotoFactor[RA] + 1;
  gotoFactor[DC] = EEPROM.readByte(DECGoto_Address);
  normalGotoSpeed[DC] = multiplierDC * gotoFactor[DC] + 1;
  
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
  Serial1.println(multiplierRA);
  Serial1.println(multiplierDC);
  Serial1.println();
#endif
  if(!checkEEPROM()){
    while(1); //prevent AstroEQ startup if EEPROM is blank.
  }
  minSpeed[RA] = synta.cmd.siderealIVal[RA]>>1;//2x sidereal rate.
  minSpeed[DC] = synta.cmd.siderealIVal[DC]>>1;//[minspeed is the point at which acceleration curves are enabled]
  calculateRate(RA); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
  calculateRate(DC); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
  gotoDecelerationLength[RA] = calculateDecelerationLength(RA);
  gotoDecelerationLength[DC] = calculateDecelerationLength(DC);
}

unsigned int calculateDecelerationLength (byte axis){
  unsigned int currentSpeed = normalGotoSpeed[axis];
  unsigned int stoppingSpeed = minSpeed[axis];
  unsigned int stepRepeat = synta.cmd.stepRepeat[axis] + 1;
  unsigned int numberOfSteps = 0;
  while(currentSpeed < stoppingSpeed) {
    currentSpeed += (currentSpeed >> 5) + 1; //work through the decelleration formula.
    numberOfSteps += stepRepeat; //n more steps
  }
  //number of steps now contains how many steps required to slow to a stop.
  if (!(numberOfSteps & 0xFF)) {
    numberOfSteps++; //If it is a multiple of 16, an assumption in the ISR calculations won't hold true. So add an extra step.
  }
  return numberOfSteps;
}

int main(void)
{
  sei();
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
#else
    for(byte j = 0; j < 3; j++){
      pinMode(modePin[j],OUTPUT);
      digitalWrite(modePin[j],state[j]);
    }
#endif
    pinMode(resetPin[i],OUTPUT); //enable the reset pin
    digitalWrite(resetPin[i],LOW); //active low reset
    _delay_ms(1); //allow ic to reset
    digitalWrite(resetPin[i],HIGH); //complete reset
    
    pinMode(st4Pin[i][0],INPUT_PULLUP);
    pinMode(st4Pin[i][1],INPUT_PULLUP);
  }
    
  // start Serial port:
  Serial.begin(9600); //SyncScan runs at 9600Baud, use a serial port of your choice 
  while(Serial.available()){
    Serial.read(); //Clear the buffer
  }
  
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

  for(;;){ //loop
    //static unsigned long lastMillis = millis();
    //static boolean isLedOn = false;
    char decodedPacket[11]; //temporary store for completed command ready to be processed
    //*digitalPinToPortReg(statusPin) &= ~_BV(digitalPinToBit(statusPin));
    if (Serial.available()) { //is there a byte in buffer
      *digitalPinToPortReg(statusPin) ^= _BV(digitalPinToBit(statusPin));
      //digitalWrite(statusPin,HIGH); //Turn on the LED to indicate activity.
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
  //    lastMillis = millis();
  //    isLedOn = true;
  //  }
  //  if (isLedOn && (millis() > lastMillis + 20)){
  //    isLedOn = false;
  //    digitalWrite(statusPin,LOW); //Turn LED back off after a short delay.
    }
    for(byte axis = 0; axis < 2; axis++){
      if(readyToGo[axis]==1){
        /*byte currentDir;
        if(synta.cmd.stepDir[axis] > 0){
          currentDir = 0;
        } else {
          currentDir = 1;
        } //convert step direction back to synta direction.
        
        //If directions are the same, we can just continue and decellerate to target speed.
        */
        //if motor is stopped, then we can accelerate to target speed.
        //Otherwise don't start the next movement until we have stopped.
        if(/*(currentDir == synta.cmd.dir[axis]) || */(synta.cmd.stopped[axis])){
          synta.cmd.updateStepDir(axis);
          if(synta.cmd.GVal[axis] & 1){
            //This is the funtion that enables a slew type move.
            slewMode(axis); //Slew type
            readyToGo[axis] = 2;
          } else {
            //This is the function for goto mode. You may need to customise it for a different motor driver
            gotoMode(axis); //Goto Mode
            readyToGo[axis] = 0;
          }
        }
      }
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
        readyToGo[axis] = 0;
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
        readyToGo[axis] = 0;
        break;
    case 'H': //set goto position, return empty response (this sets the number of steps to move from cuurent position if in goto mode)
        synta.cmd.setHVal(axis, synta.hexToLong(packetIn)); //set the goto position container (convert string to long first)
        readyToGo[axis] = 0;
        break;
    case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
        responseData = synta.hexToLong(packetIn);
        //if (responseData > minSpeed[axis]){
        //  responseData = minSpeed[axis]; //minimum speed limit <- if it is slower than this, there will be no acelleration.
        //}
        synta.cmd.setIVal(axis, responseData); //set the speed container (convert string to long first) 
        responseData = 0;
        if (readyToGo[axis] == 2){
          motorStart(axis,1); //Update Speed.
        } else {
          readyToGo[axis] = 0;
        }
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
#if DEBUG == 2
  else {
    Serial.println(); 
  }
#endif
#endif
  
  if(command == 'J'){ //J tells us we are ready to begin the requested movement. So signal we are ready to go and when the last movement complets this one will execute.
    readyToGo[axis] = 1;
    if (!(synta.cmd.GVal[axis] & 1)){
      synta.cmd.setGotoEn(axis,1);
    }
  }
}



//Calculates the rate based on the recieved I value 
void calculateRate(byte motor){
  
  unsigned long rate;
  unsigned long remainder;
  float floatRemainder;
  unsigned long divisor = synta.cmd.bVal[motor];
  byte distWidth;
 // byte GVal = synta.cmd.GVal[motor];
  //if((GVal == 2)||(GVal == 1)){ //Normal Speed}
  if(!highSpeed[motor]){ //Normal Speed
  //  highSpeed[motor] = false;
    distWidth = NormalDistnWidth;
  } else { //High Speed
  //  highSpeed[motor] = true;
    distWidth = HighspeedDistnWidth; //There are 8 times fewer steps over which to distribute extra clock cycles
  }
  
  //When dividing a very large number by a much smaller on, float accuracy is abismal. So firstly we use integer math to split the division into quotient and remainder
  rate = timerCountRate / divisor; //Calculate the quotient
  remainder = timerCountRate % divisor; //Calculate the remainder
  
  //Then convert the remainder into a decimal number (division of a small number by a larger one, improving accuracy)
  floatRemainder = (float)remainder/(float)divisor; //Convert the remainder to a decimal.
    
  //Multiply the remainder by distributionWidth to work out an approximate number of extra clocks needed per full step (each step is 'distributionWidth' microsteps)
  floatRemainder *= (float)distWidth; 
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
  
  for (byte i = 0; i < distWidth; i++){
#if defined(__AVR_ATmega162__)
    timerOVF[motor][i] = rate; //Subtract 1 as timer is 0 indexed.
#else
    timerOVF[motor][i] = rate; //Hmm, for some reason this one doesn't need 1 subtracting???
#endif
  }
  
  //evenly distribute the required number of extra clocks over the full step.
  for (unsigned long i = 0; i < remainder; i++){
    float distn = i;
    distn *= (float)distWidth;
    distn /= (float)remainder;
    byte index = (byte)ceil(distn);
    timerOVF[motor][index] += 1;
  }
  
#ifdef DEBUG
  Serial1.println();
  Serial1.println(rate);
  Serial1.println();
#endif
  
  distributionWidth[motor] = distWidth - 1; //decrement to get its bitmask. (distnWidth is always a power of 2)
  
  byte x = synta.cmd.siderealIVal[motor]>>3;
  byte y = ((unsigned int)rate)>>3;
  unsigned int divisior = x * y;
  synta.cmd.stepRepeat[motor] = (byte)((18750+(divisior>>1))/divisior)-1; //note that we are rounding to neareast so we add half the divisor.
#ifdef DEBUG
  Serial1.println();
  Serial1.println(synta.cmd.stepRepeat[motor] );
  Serial1.println();
#endif
}

void motorEnable(byte axis){
  digitalWrite(enablePin[axis],LOW);
  synta.cmd.setFVal(axis,1);
  configureTimer(); //setup the motor pulse timers.
}

void motorDisable(byte axis){
  digitalWrite(enablePin[axis],LOW);
  synta.cmd.setFVal(axis,0);
}

void slewMode(byte axis){
  motorStart(axis,1); //Begin PWM
}

void gotoMode(byte axis){
  unsigned int decelerationLength = gotoDecelerationLength[axis];
  if (axis == RA){
    if (synta.cmd.HVal[axis] < (decelerationLength>>1)){
      synta.cmd.setHVal(axis,(decelerationLength>>1));
    }
  } else {
    if (synta.cmd.HVal[axis] < 10){
      synta.cmd.setHVal(axis,10);
    }
  }
  unsigned long HVal = synta.cmd.HVal[axis];
  unsigned long halfHVal = (HVal >> 1) + 1; //make decel slightly longer than accel
  unsigned int gotoSpeed = normalGotoSpeed[axis];
  if (halfHVal < decelerationLength) {
    decelerationLength = halfHVal;
  }
#ifdef DEBUG
  Serial1.println();
  Serial1.println(decelerationLength);
  Serial1.println(synta.cmd.jVal[axis]);
#endif
  HVal -= decelerationLength;
  /*if (axis == RA){
    HVal += (decelerationLength>>2);
  }*/
  gotoPosn[axis] = synta.cmd.jVal[axis] + (synta.cmd.stepDir[axis] * HVal); //current position + relative change - decelleration region
    
#ifdef DEBUG
  Serial1.println(gotoPosn[axis]);
  Serial1.println();
#endif
  synta.cmd.setIVal(axis, gotoSpeed);
  //calculateRate(axis);
  
  motorStart(axis,decelerationLength); //Begin PWM
  gotoRunning[axis] = 1; //start the goto.
}

inline void timerEnable(byte motor, byte mode) {
  //if (mode){
    //FCPU/8
  //  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn0)); //0x0
  //  timerPrescalarRegister(motor) |= (1<<CSn1);//x1x
  //} else {
    //FCPU/1
  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn1));//00x
  timerPrescalarRegister(motor) |= (1<<CSn0);//xx1
  //}
  //synta.cmd.setStepLength(motor, 1);//(mode ? synta.cmd.gVal[motor] : 1));
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

//As there is plenty of FLASH left, then to improve speed, I have created two motorStart functions (one for RA and one for DEC)
void motorStart(byte motor, unsigned int gotoDeceleration){
  if (motor == RA) {
    motorStartRA(gotoDeceleration);
  } else {
    motorStartDC(gotoDeceleration);
  }
}
void motorStartRA(unsigned int gotoDeceleration){
  unsigned int IVal = synta.cmd.IVal[RA];
  unsigned int currentIVal;
  interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
  currentIVal = currentMotorSpeed(RA);
  interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
#ifdef DEBUG
  Serial1.println(F("IVal:"));
  Serial1.println(IVal);
  Serial1.println();
#endif
  unsigned int startSpeed;
  unsigned int stoppingSpeed;
  if (IVal > minSpeed[RA]){
    stoppingSpeed = IVal;
  } else {
    stoppingSpeed = minSpeed[RA];
  }
  if(synta.cmd.stopped[RA]) {
    startSpeed = stoppingSpeed;
  } else {
    if (currentIVal < minSpeed[RA]) {
      startSpeed = currentIVal;
    } else {
      startSpeed = stoppingSpeed;
    }
  }
 
  interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
  synta.cmd.currentIVal[RA] = synta.cmd.IVal[RA];
  currentMotorSpeed(RA) = startSpeed;
  stopSpeed[RA] = stoppingSpeed;
  interruptCount(RA) = 1;
  if (encodeDirection[RA]^synta.cmd.dir[RA]) {
    *digitalPinToPortReg(dirPin[RA]) |= _BV(digitalPinToBit(dirPin[RA]));
  } else {
    *digitalPinToPortReg(dirPin[RA]) &= ~_BV(digitalPinToBit(dirPin[RA]));
  }
  if(synta.cmd.stopped[RA]) { //if stopped, configure timers
    //digitalWrite(dirPin[RA],encodeDirection[RA]^synta.cmd.dir[RA]); //set the direction
    stepIncrementRepeat[RA] = 0;
    distributionSegment(RA) = 0;
    int* decelerationSteps = (int*)&decelerationStepsLow(RA); //low and high are in sequential registers so we can treat them as an int in the sram.
    *decelerationSteps = -gotoDeceleration;
    timerCountRegister(RA) = 0;
    interruptOVFCount(RA) = timerOVF[RA][0];
    timerEnable(RA,highSpeed[RA]);
    synta.cmd.setStopped(RA, 0);
  }
  interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
#ifdef DEBUG
  Serial1.println(F("StartSpeed:"));
  Serial1.println(startSpeed);
  Serial1.println();
#endif
}

void motorStartDC(unsigned int gotoDeceleration){
  unsigned int IVal = synta.cmd.IVal[DC];
  unsigned int currentIVal;
  interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
  currentIVal = currentMotorSpeed(DC);
  interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt
#ifdef DEBUG
  Serial1.println(F("IVal:"));
  Serial1.println(IVal);
  Serial1.println();
#endif
  unsigned int startSpeed;
  unsigned int stoppingSpeed;
  if (IVal > minSpeed[DC]){
    stoppingSpeed = IVal;
  } else {
    stoppingSpeed = minSpeed[DC];
  }
  if(synta.cmd.stopped[DC]) {
    startSpeed = stoppingSpeed;
  } else {
    if (currentIVal < minSpeed[DC]) {
      startSpeed = currentIVal;
    } else {
      startSpeed = stoppingSpeed;
    }
  }
 
  interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
  synta.cmd.currentIVal[DC] = synta.cmd.IVal[DC];
  currentMotorSpeed(DC) = startSpeed;
  stopSpeed[DC] = stoppingSpeed;
  interruptCount(DC) = 1;
  if (encodeDirection[DC]^synta.cmd.dir[DC]) {
    *digitalPinToPortReg(dirPin[DC]) |= _BV(digitalPinToBit(dirPin[DC]));
  } else {
    *digitalPinToPortReg(dirPin[DC]) &= ~_BV(digitalPinToBit(dirPin[DC]));
  }
  if(synta.cmd.stopped[DC]) { //if stopped, configure timers
    //digitalWrite(dirPin[DC],encodeDirection[DC]^synta.cmd.dir[DC]); //set the direction
    stepIncrementRepeat[DC] = 0;
    distributionSegment(DC) = 0;
    int* decelerationSteps = (int*)&decelerationStepsLow(DC); //low and high are in sequential registers so we can treat them as an int in the sram.
    *decelerationSteps = -gotoDeceleration;
    timerCountRegister(DC) = 0;
    interruptOVFCount(DC) = timerOVF[DC][0];
    timerEnable(DC,highSpeed[DC]);
    synta.cmd.setStopped(DC, 0);
  }
  interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt
#ifdef DEBUG
  Serial1.println(F("StartSpeed:"));
  Serial1.println(startSpeed);
  Serial1.println();
#endif
}

//As there is plenty of FLASH left, then to improve speed, I have created two motorStop functions (one for RA and one for DEC)
void motorStop(byte motor, byte emergency){
  if (motor == RA) {
    motorStopRA(emergency);
  } else {
    motorStopDC(emergency);
  }
}

void motorStopRA(byte emergency){
  if (emergency) {
    //trigger instant shutdown of the motor in an emergency.
    timerDisable(RA);
    synta.cmd.setGotoEn(RA,0); //Not in goto mode.
    synta.cmd.setStopped(RA,1); //mark as stopped
    readyToGo[RA] = 0;
    gotoRunning[RA] = 0;
  } else if (!synta.cmd.stopped[RA]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
    //trigger ISR based decelleration
    //readyToGo[RA] = 0;
    synta.cmd.setGotoEn(RA,0); //No longer in goto mode.
    gotoRunning[RA] = 0;
    interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
    if(synta.cmd.currentIVal[RA] < minSpeed[RA]){
      if(stopSpeed[RA] > minSpeed[RA]){
        stopSpeed[RA] = minSpeed[RA];
      }
    }/* else {
      stopSpeed[RA] = synta.cmd.currentIVal[RA];
    }*/
    synta.cmd.currentIVal[RA] = stopSpeed[RA] + 1;//synta.cmd.stepIncrement[motor];
    interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
  }
}

void motorStopDC(byte emergency){
  if (emergency) {
    //trigger instant shutdown of the motor in an emergency.
    timerDisable(DC);
    synta.cmd.setGotoEn(DC,0); //Not in goto mode.
    synta.cmd.setStopped(DC,1); //mark as stopped
    readyToGo[DC] = 0;
    gotoRunning[DC] = 0;
  } else if (!synta.cmd.stopped[DC]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
    //trigger ISR based decelleration
    //readyToGo[motor] = 0;
    synta.cmd.setGotoEn(DC,0); //No longer in goto mode.
    gotoRunning[DC] = 0;
    interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
    if(synta.cmd.currentIVal[DC] < minSpeed[DC]){
      if(stopSpeed[DC] > minSpeed[DC]){
        stopSpeed[DC] = minSpeed[DC];
      }
    }/* else {
      stopSpeed[DC] = synta.cmd.currentIVal[DC];
    }*/
    synta.cmd.currentIVal[DC] = stopSpeed[DC] + 1;//synta.cmd.stepIncrement[motor];
    interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt
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
      synta.cmd.currentIVal[RA] = newSpeed;
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
        if (stopSpeed[RA] < synta.cmd.currentIVal[RA]) {
          stopSpeed[RA] = synta.cmd.currentIVal[RA]; //ensure that RA doesn't stop.
        }
      }
    } else {
ignoreRAST4:
      synta.cmd.currentIVal[RA] = newSpeed;
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
      synta.cmd.currentIVal[DC] = (synta.cmd.siderealIVal[DC] << 2); //move at 0.25x sidereal rate
      synta.cmd.GVal[DC] = 1; //slew mode
      //calculateRate(DC);
      motorStart(DC,1);
    } else {
      synta.cmd.currentIVal[DC] = stopSpeed[DC] + 1;//make our target >stopSpeed so that ISRs bring us to a halt.
    }
  }
}

/*Timer Interrupt Vector*/
ISR(TIMER3_CAPT_vect, ISR_NAKED) {
  asm volatile (
    "push r24       \n\t"
    "in   r24, %0   \n\t" 
    "push r24       \n\t"
    "push r25       \n\t"
    :
    : "I" (_SFR_IO_ADDR(SREG))
  );
  
  unsigned int count = interruptCount(DC)-1; //OCR1B is used as it is an unused register which affords us quick access.
  if (count == 0) {
    asm volatile (
      "push r30       \n\t"
      "push r31       \n\t"
      "push r18       \n\t"
      "push r19       \n\t"
      "push r21       \n\t"
      "push r20       \n\t"
      "push  r0       \n\t"
      "push  r1       \n\t"
      "eor   r1,r1    \n\t"
    );
    byte timeSegment = distributionSegment(DC);
    byte index = (distributionWidth[DC] << 1) & timeSegment;
    interruptOVFCount(DC) = *(int*)((byte*)timerOVF[DC] + index); //move to next pwm period
    distributionSegment(DC) = timeSegment + 1;
    
    register byte port asm("r18") = stepPort(DC);
    register unsigned int startVal asm("r24") = currentMotorSpeed(DC);
    //byte port = STEP0PORT;
    //unsigned int startVal = currentMotorSpeed(DC);
    interruptCount(DC) = startVal; //start val is used for specifying how many interrupts between steps. This tends to IVal after acceleration
    if (port & stepHigh(DC)){
      asm volatile (
        "push r27       \n\t"
        "push r26       \n\t"
      );
      stepPort(DC) = port & stepLow(DC);
      unsigned long jVal = synta.cmd.jVal[DC];
      jVal = jVal + synta.cmd.stepDir[DC];
      synta.cmd.jVal[DC] = jVal;
      if(gotoRunning[DC]){
        if (gotoPosn[DC] == jVal){ //reached the decelleration marker. Note that this line loads gotoPosn[] into r24:r27
          //will decellerate the rest of the way. So first move gotoPosn[] to end point.
          if (decelerationStepsHigh(DC) & _BV(7)) {
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
            register char stepDir asm("r20") = synta.cmd.stepDir[DC];
            register unsigned long newGotoPosn asm("r18");
            asm volatile(
              "in   %A0, %2   \n\t"
              "in   %B0, %3   \n\t"
              "cp   %1, __zero_reg__  \n\t"
              "brlt .+6       \n\t"
              "neg  %A0       \n\t"
              "com  %B0       \n\t" //while this is not strictly accurate (works except when num=512), it is fine for this.
              "eor  %C0, %C0  \n\t"
              "mov  %D0, %C0  \n\t"
              "add  %A0, r24  \n\t" //add previously loaded gotoPosn[] registers to new gotoPosn[] registers.
              "adc  %B0, r25  \n\t"
              "adc  %C0, r26  \n\t"
              "adc  %D0, r27  \n\t"
              : "=a" (newGotoPosn) //goto selects r18:r21. This adds sts commands for all 4 bytes
              : "r" (stepDir),"I" (_SFR_IO_ADDR(decelerationStepsLow(DC))),"I" (_SFR_IO_ADDR(decelerationStepsHigh(DC)))       //stepDir is in r19 to match second byte of goto.
              :
            );
            gotoPosn[DC] = newGotoPosn;
            //-----------------------------------------------------------------------------------------
            decelerationStepsHigh(DC) = 0; //say we are stopping
            synta.cmd.currentIVal[DC] = stopSpeed[DC]; //decellerate to min speed.
          } else {
            goto stopMotorISR3;
          }
        }
      }
      if (currentMotorSpeed(DC) > stopSpeed[DC]) {
stopMotorISR3:
        if(gotoRunning[DC]){ //if we are currently running a goto then 
          synta.cmd.gotoEn[DC] = 0; //Goto mode complete
          gotoRunning[DC] = 0; //Goto no longer running
        } //otherwise don't as it cancels a 'goto ready' state
        synta.cmd.stopped[DC] = 1; //mark as stopped
        timerDisable(DC);
      }
      asm volatile (
        "pop  r26       \n\t"
        "pop  r27       \n\t"
      );
    } else {
      stepPort(DC) = port | stepHigh(DC);
      register unsigned int iVal asm("r20") = synta.cmd.currentIVal[DC];
      if (iVal != startVal){
        port = stepIncrementRepeat[DC];
        register byte repeat asm("r19") = synta.cmd.stepRepeat[DC];
        if (repeat == port){
          register byte increment asm("r0");// = synta.cmd.stepIncrement[DC];
          /*
            stepIncrement[DC] = (startVal >> 5) + 1;
          */
          asm volatile(
            "mov   %1,%A2  \n\t"
            "mov   %0,%B2  \n\t"
            "andi  %1,0xF0 \n\t"
            "swap  %0      \n\t"
            "swap  %1      \n\t"
            "or    %0,%1   \n\t"
            "lsr   %0      \n\t"
            "inc   %0      \n\t"
            : "=&t" (increment) //increment is in r0
            : "r" (repeat), "w" (startVal) //startVal is in r24:25
            : 
          );
          /*
          if (startVal - increment >= iVal) { // if (iVal <= startVal-increment)
            startVal = startVal - increment;
          } else if (startVal + increment <= iVal){
            startVal = startVal + increment;
          } else {
            startVal = iVal;
          }
          currentMotorSpeed(DC) = startVal;
          stepIncrementRepeat[DC] = 0;
          */
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
          port = 0;
        } else {
          /*
            stepIncrementRepeat[DC]++;
          */
          port++;
        }
        stepIncrementRepeat[DC] = port;
      }
    }
    asm volatile (
      "pop   r1       \n\t"
      "pop   r0       \n\t"
      "pop  r20       \n\t"
      "pop  r21       \n\t"
      "pop  r19       \n\t"
      "pop  r18       \n\t"
      "pop  r31       \n\t"
      "pop  r30       \n\t"
    );
  } else {
    interruptCount(DC) = count;
  }
  asm volatile (
    "pop  r25       \n\t"
    "pop  r24       \n\t"
    "out   %0,r24   \n\t" 
    "pop  r24       \n\t"
    "reti           \n\t"
    :
    : "I" (_SFR_IO_ADDR(SREG))
  );
}

/*Timer Interrupt Vector*/
ISR(TIMER1_CAPT_vect, ISR_NAKED) {
  asm volatile (
    "push r24       \n\t"
    "in   r24, %0   \n\t" 
    "push r24       \n\t"
    "push r25       \n\t"
    :
    : "I" (_SFR_IO_ADDR(SREG))
  );
  
  unsigned int count = interruptCount(RA)-1;
  if (count == 0) {
    asm volatile (
      "push r30       \n\t"
      "push r31       \n\t"
      "push r18       \n\t"
      "push r19       \n\t"
      "push r21       \n\t"
      "push r20       \n\t"
      "push  r0       \n\t"
      "push  r1       \n\t"
      "eor   r1,r1    \n\t"
    );
    byte timeSegment = distributionSegment(RA);
    byte index = (distributionWidth[RA] << 1) & timeSegment;
    interruptOVFCount(RA) = *(int*)((byte*)timerOVF[RA] + index); //move to next pwm period
    distributionSegment(RA) = timeSegment + 1;
    
    register byte port asm("r18") = stepPort(RA);
    register unsigned int startVal asm("r24") = currentMotorSpeed(RA);
    //byte port = STEP1PORT;
    //unsigned int startVal = currentMotorSpeed(RA);
    interruptCount(RA) = startVal; //start val is used for specifying how many interrupts between steps. This tends to IVal after acceleration
    if (port & stepHigh(RA)){
      asm volatile (
        "push r27       \n\t"
        "push r26       \n\t"
      );
      stepPort(RA) = port & stepLow(RA);
      unsigned long jVal = synta.cmd.jVal[RA];
      jVal = jVal + synta.cmd.stepDir[RA];
      synta.cmd.jVal[RA] = jVal;
      if(gotoRunning[RA]){
        if (jVal == gotoPosn[RA]){ //reached the decelleration marker
          //will decellerate the rest of the way. So first move gotoPosn[] to end point.
          if (decelerationStepsHigh(RA) & _BV(7)) {
            /*
            unsigned long gotoPos = gotoPosn[RA];
            if (synta.cmd.stepDir[RA] < 0){
              gotoPosn[RA] = gotoPos + decelerationSteps(RA);
            } else {
              gotoPosn[RA] = gotoPos - decelerationSteps(RA);
            }
            */
            //--- This is a heavily optimised version of the code commented out just above ------------
            register char stepDir asm("r20") = synta.cmd.stepDir[RA];
            register unsigned long newGotoPosn asm("r18");
            asm volatile(
              "in   %A0, %2   \n\t"
              "in   %B0, %3   \n\t"
              "cp   %1, __zero_reg__  \n\t"
              "brlt .+6       \n\t"
              "neg  %A0       \n\t"
              "com  %B0       \n\t" //while this is not strictly accurate (works except when num=512), it is fine for this.
              "eor  %C0, %C0  \n\t"
              "mov  %D0, %C0  \n\t"
              "add  %A0, r24  \n\t"
              "adc  %B0, r25  \n\t"
              "adc  %C0, r26  \n\t"
              "adc  %D0, r27  \n\t"
              : "=a" (newGotoPosn) //goto selects r18:r21. This adds sts commands for all 4 bytes
              : "r" (stepDir),"I" (_SFR_IO_ADDR(decelerationStepsLow(RA))), "I" (_SFR_IO_ADDR(decelerationStepsHigh(RA)))       //temp is in r19 to match second byte of goto.
              :
            );
            gotoPosn[RA] = newGotoPosn;
            decelerationStepsHigh(RA) = 0; //say we are stopping
            synta.cmd.currentIVal[RA] = stopSpeed[RA]; //decellerate to min speed. This is possible in at most 80 steps.
          } else {
            goto stopMotorISR1;
          }
        }
      }
      if (currentMotorSpeed(RA) > stopSpeed[RA]) {
stopMotorISR1:
        if(gotoRunning[RA]){ //if we are currently running a goto then 
          synta.cmd.gotoEn[RA] = 0; //Goto mode complete
          gotoRunning[RA] = 0; //Goto complete.
        } //otherwise don't as it cancels a 'goto ready' state
        synta.cmd.stopped[RA] = 1; //mark as stopped
        timerDisable(RA);
      }
      asm volatile (
        "pop  r26       \n\t"
        "pop  r27       \n\t"
      );
    } else {
      stepPort(RA) = port | stepHigh(RA);
      register unsigned int iVal asm("r20") = synta.cmd.currentIVal[RA];
      if (iVal != startVal){
        port = stepIncrementRepeat[RA];
        register byte repeat asm("r19") = synta.cmd.stepRepeat[RA];
        if (repeat == port){
          register byte increment asm("r0");// = synta.cmd.stepIncrement[RA];
          /*
            stepIncrement[RA] = (startVal >> 5) + 1;
          */
          asm volatile(
            "mov   %1,%A2  \n\t"
            "mov   %0,%B2  \n\t"
            "andi  %1,0xF0 \n\t"
            "swap  %0      \n\t"
            "swap  %1      \n\t"
            "or    %0,%1   \n\t"
            "lsr   %0      \n\t"
            "inc   %0      \n\t"
            : "=&t" (increment) //increment is in r0
            : "r" (repeat), "w" (startVal) //startVal is in r24:25
            : 
          );
          /*
          if (startVal - increment >= iVal) { // if (iVal <= startVal-increment)
            startVal = startVal - increment;
          } else if (startVal + increment <= iVal){
            startVal = startVal + increment;
          } else {
            startVal = iVal;
          }
          currentMotorSpeed(RA) = startVal;
          stepIncrementRepeat[RA] = 0;
          */
          asm volatile(
            "movw r18, %0    \n\t"
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
          port = 0;
        } else {
          /*
            stepIncrementRepeat++;
          */
          port++;
        }
        stepIncrementRepeat[RA] = port;
      }
    }
    asm volatile (
      "pop   r1       \n\t"
      "pop   r0       \n\t"
      "pop  r20       \n\t"
      "pop  r21       \n\t"
      "pop  r19       \n\t"
      "pop  r18       \n\t"
      "pop  r31       \n\t"
      "pop  r30       \n\t"
    );
  } else {
    interruptCount(RA) = count;
  }
  asm volatile (
    "pop  r25       \n\t"
    "pop  r24       \n\t"
    "out   %0,r24   \n\t" 
    "pop  r24       \n\t"
    "reti           \n\t"
    :
    : "I" (_SFR_IO_ADDR(SREG))
  );
}

#else
#error Unsupported Part! Please use an Arduino Mega, or ATMega162
#endif

