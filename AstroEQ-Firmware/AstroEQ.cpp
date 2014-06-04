/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Syntia
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts
 
  Current Verison: 7.3
*/

//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#include "AstroEQ.h"

#include "EEPROMReader.h" //Read config file
#include "SerialLink.h" //Serial Port
#include "UnionHelpers.h"
#include "synta.h" //Synta Communications Protocol.
#include <util/delay.h>    
#include <avr/wdt.h>

byte stepIncrement[2];
unsigned int normalGotoSpeed[2];
unsigned int minSpeed[2];
unsigned int stopSpeed[2];
byte readyToGo[2] = {0,0};
volatile byte gotoRunning[2] = {0,0};
unsigned long gotoPosn[2] = {0UL,0UL}; //where to slew to

#define timerCountRate 8000000

#define DecimalDistnWidth 32
unsigned int timerOVF[2][DecimalDistnWidth];
bool canJumpToHighspeed = false;
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
#define gotoCompleteBitMask(m) (m ? _BV(3) : _BV(2))
#define gotoCompleteRegister GPIOR2

//Pins
const byte statusPin = statusPin_Define;
const byte resetPin[2] = {resetPin_0_Define,resetPin_1_Define};
const byte dirPin[2] = {dirPin_0_Define,dirPin_1_Define};
const byte enablePin[2] = {enablePin_0_Define,enablePin_1_Define};
const byte stepPin[2] = {stepPin_0_Define,stepPin_1_Define};
const byte st4Pin[2][2] = {{ST4AddPin_0_Define,ST4SubPin_0_Define},{ST4AddPin_1_Define,ST4SubPin_1_Define}};

const byte modePins[2][3] = {{modePins0_0_Define,modePins1_0_Define,modePins2_0_Define},{modePins0_1_Define,modePins1_1_Define,modePins2_1_Define}};
 

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

byte modeState[2][3] = {{HIGH,HIGH,HIGH},{HIGH,LOW,LOW}};
#define MODE0 0
#define MODE1 1
#define MODE2 2
#define STATE16 0
#define STATE2 1

void buildModeMapping(byte microsteps, bool driverVersion){
  if (microsteps < 8){
    microsteps *= 8;
  }
  if (microsteps <16){
      modeState[STATE16][MODE0] = HIGH;
      modeState[STATE16][MODE1] = HIGH;
      modeState[STATE16][MODE2] = LOW;
      modeState[STATE2][MODE0] = LOW;
      modeState[STATE2][MODE1] = LOW;
      modeState[STATE2][MODE2] = LOW;
  } else if (microsteps > 16){
      modeState[STATE16][MODE0] = HIGH;
      modeState[STATE16][MODE1] = HIGH;
      modeState[STATE16][MODE2] = HIGH;
      modeState[STATE2][MODE0] = LOW;
      modeState[STATE2][MODE1] = HIGH;
      modeState[STATE2][MODE2] = LOW;
  } else {
      modeState[STATE16][MODE0] = driverVersion ? LOW : HIGH;
      modeState[STATE16][MODE1] = driverVersion ? LOW : HIGH;
      modeState[STATE16][MODE2] = HIGH;
      modeState[STATE2][MODE0] = HIGH;
      modeState[STATE2][MODE1] = LOW;
      modeState[STATE2][MODE2] = LOW;
  }
}





byte progMode = 0; //MODES:  0 = Normal Ops (EQMOD). 1 = Validate EEPROM. 2 = Store to EEPROM. 3 = Rebuild EEPROM
byte microstepConf;
byte driverVersion;

bool checkEEPROM(){
  char temp[9] = {0};
  EEPROM_readString(temp,8,AstroEQID_Address);
  if(strncmp(temp,"AstroEQ",8)){
    return false;
  }
  if ((encodeDirection[RA] > 1) || (encodeDirection[DC] > 1)){
    return false; //invalid value.
  }
  if (driverVersion > 1){
    return false; //invalid value.
  }
  if (driverVersion && microstepConf > 32){
    return false; //invalid value.
  } else if (!driverVersion && microstepConf > 16){
    return false; //invalid value.
  }
  if ((cmd.siderealIVal[RA] > 1200) || (cmd.siderealIVal[RA] < 300)) {
    return false; //invalid value.
  }
  if ((cmd.siderealIVal[DC] > 1200) || (cmd.siderealIVal[DC] < 300)) {
    return false; //invalid value.
  }
  if(normalGotoSpeed[RA] == 0){
    return false; //invalid value.
  }
  if(normalGotoSpeed[DC] == 0){
    return false; //invalid value.
  }
  return true;
}

void buildEEPROM(){
  EEPROM_writeString("AstroEQ",8,AstroEQID_Address);
}

void storeEEPROM(){
  EEPROM_writeLong(cmd.aVal[RA],aVal1_Address);
  EEPROM_writeLong(cmd.aVal[DC],aVal2_Address);
  EEPROM_writeLong(cmd.bVal[RA],bVal1_Address);
  EEPROM_writeLong(cmd.bVal[DC],bVal2_Address);
  EEPROM_writeLong(cmd.sVal[RA],sVal1_Address);
  EEPROM_writeLong(cmd.sVal[DC],sVal2_Address);
  EEPROM_writeByte(encodeDirection[RA],RAReverse_Address);
  EEPROM_writeByte(encodeDirection[DC],DECReverse_Address);
  EEPROM_writeByte(driverVersion,Driver_Address);
  EEPROM_writeByte(microstepConf,Microstep_Address);
  EEPROM_writeByte(normalGotoSpeed[RA],RAGoto_Address);
  EEPROM_writeByte(normalGotoSpeed[DC],DECGoto_Address);
  EEPROM_writeInt(cmd.siderealIVal[RA],IVal1_Address);
  EEPROM_writeInt(cmd.siderealIVal[DC],IVal2_Address);
}

void systemInitialiser(){
  synta_initialise(1281); //initialise mount instance, specify version!
  
  encodeDirection[RA] = EEPROM_readByte(RAReverse_Address);  //reverse the right ascension if 1
  encodeDirection[DC] = EEPROM_readByte(DECReverse_Address); //reverse the declination if 1
  
  driverVersion = EEPROM_readByte(Driver_Address);
  
  normalGotoSpeed[RA] = EEPROM_readByte(RAGoto_Address);
  normalGotoSpeed[DC] = EEPROM_readByte(DECGoto_Address);
  
  microstepConf = EEPROM_readByte(Microstep_Address);
  buildModeMapping(microstepConf, driverVersion);
  
  canJumpToHighspeed = microstepConf>4;
  
  if(!checkEEPROM()){
    progMode = 1; //prevent AstroEQ startup if EEPROM is blank.
  }
  minSpeed[RA] = cmd.siderealIVal[RA]>>1;//2x sidereal rate.
  minSpeed[DC] = cmd.siderealIVal[DC]>>1;//[minspeed is the point at which acceleration curves are enabled]
  calculateRate(RA); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
  calculateRate(DC); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
  gotoDecelerationLength[RA] = calculateDecelerationLength(RA);
  gotoDecelerationLength[DC] = calculateDecelerationLength(DC);
}

unsigned int calculateDecelerationLength (byte axis){
  unsigned int currentSpeed = normalGotoSpeed[axis];
  unsigned int stoppingSpeed = minSpeed[axis];
  unsigned int stepRepeat = cmd.stepRepeat[axis] + 1;
  unsigned int numberOfSteps = 0;
  while(currentSpeed < stoppingSpeed) {
    currentSpeed += (currentSpeed >> 5) + 1; //work through the decelleration formula.
    numberOfSteps += stepRepeat; //n more steps
  }
  //number of steps now contains how many steps required to slow to a stop.
  return numberOfSteps;
}

int main(void)
{
  sei();
  systemInitialiser();
#if defined(__AVR_ATmega162__)
  //Timer 0 and 2 registers are being used as general purpose data storage for high efficency interrupt routines. So timer must be fully disabled.
  TCCR2 = 0;
  ASSR=0;
  GPIOR2 = 0;
  TIMSK &= ~(_BV(TOIE0) | _BV(OCIE0));
  TCCR0 = 0;
#else
  //Timer 0 registers are being used as general purpuse data storage. Timer must be fully disabled.
  TCCR0A = 0;
  TCCR0B = 0;
  TIMSK0 = 0;
#endif
  //Status pins to output
  *digitalPinToDirectionReg(statusPin) |= _BV(digitalPinToBit(statusPin));
  //Reset pins to output
  *digitalPinToDirectionReg(resetPin[RA]) |= _BV(digitalPinToBit(resetPin[RA]));
  *digitalPinToPortReg(resetPin[RA]) &= ~_BV(digitalPinToBit(resetPin[RA]));
  *digitalPinToDirectionReg(resetPin[DC]) |= _BV(digitalPinToBit(resetPin[DC]));
  *digitalPinToPortReg(resetPin[DC]) &= ~_BV(digitalPinToBit(resetPin[DC])); 
  //Enable pins to output
  *digitalPinToDirectionReg(enablePin[RA]) |= _BV(digitalPinToBit(enablePin[RA]));
  *digitalPinToPortReg(enablePin[RA]) |= _BV(digitalPinToBit(enablePin[RA])); //IC disabled
  *digitalPinToDirectionReg(enablePin[DC]) |= _BV(digitalPinToBit(enablePin[DC]));
  *digitalPinToPortReg(enablePin[DC]) |= _BV(digitalPinToBit(enablePin[DC])); //IC disabled
  //Step pins to output
  *digitalPinToDirectionReg(stepPin[RA]) |= _BV(digitalPinToBit(stepPin[RA]));
  *digitalPinToPortReg(stepPin[RA]) &= ~_BV(digitalPinToBit(stepPin[RA]));
  *digitalPinToDirectionReg(stepPin[DC]) |= _BV(digitalPinToBit(stepPin[DC]));
  *digitalPinToPortReg(stepPin[DC]) &= ~_BV(digitalPinToBit(stepPin[DC])); 
  //Direction pins to output
  *digitalPinToDirectionReg(dirPin[RA]) |= _BV(digitalPinToBit(dirPin[RA]));
  *digitalPinToPortReg(dirPin[RA]) &= ~_BV(digitalPinToBit(dirPin[RA]));
  *digitalPinToDirectionReg(dirPin[DC]) |= _BV(digitalPinToBit(dirPin[DC]));
  *digitalPinToPortReg(dirPin[DC]) &= ~_BV(digitalPinToBit(dirPin[DC])); 
  
  //Mode pins to output.
  *digitalPinToDirectionReg(modePins[RA][MODE0]) |= _BV(digitalPinToBit(modePins[RA][MODE0]));
  *digitalPinToDirectionReg(modePins[RA][MODE1]) |= _BV(digitalPinToBit(modePins[RA][MODE1]));
  *digitalPinToDirectionReg(modePins[RA][MODE2]) |= _BV(digitalPinToBit(modePins[RA][MODE2]));
  *digitalPinToDirectionReg(modePins[DC][MODE0]) |= _BV(digitalPinToBit(modePins[DC][MODE0]));
  *digitalPinToDirectionReg(modePins[DC][MODE1]) |= _BV(digitalPinToBit(modePins[DC][MODE1]));
  *digitalPinToDirectionReg(modePins[DC][MODE2]) |= _BV(digitalPinToBit(modePins[DC][MODE2]));
  const byte * state;
  if (!canJumpToHighspeed) {
    state = modeState[STATE2];
  } else {
    state = modeState[STATE16];
  }
  if (state[MODE0]) {
    *digitalPinToPortReg(modePins[RA][MODE0]) |= _BV(digitalPinToBit(modePins[RA][MODE0]));
    *digitalPinToPortReg(modePins[DC][MODE0]) |= _BV(digitalPinToBit(modePins[DC][MODE0])); 
  } else {
    *digitalPinToPortReg(modePins[RA][MODE0]) &= ~_BV(digitalPinToBit(modePins[RA][MODE0]));
    *digitalPinToPortReg(modePins[DC][MODE0]) &= ~_BV(digitalPinToBit(modePins[DC][MODE0])); 
  }
  if (state[MODE1]) {
    *digitalPinToPortReg(modePins[RA][MODE1]) |= _BV(digitalPinToBit(modePins[RA][MODE1]));
    *digitalPinToPortReg(modePins[DC][MODE1]) |= _BV(digitalPinToBit(modePins[DC][MODE1])); 
  } else {
    *digitalPinToPortReg(modePins[RA][MODE1]) &= ~_BV(digitalPinToBit(modePins[RA][MODE1]));
    *digitalPinToPortReg(modePins[DC][MODE1]) &= ~_BV(digitalPinToBit(modePins[DC][MODE1])); 
  }
  if (state[MODE2]) {
    *digitalPinToPortReg(modePins[RA][MODE2]) |= _BV(digitalPinToBit(modePins[RA][MODE2]));
    *digitalPinToPortReg(modePins[DC][MODE2]) |= _BV(digitalPinToBit(modePins[DC][MODE2])); 
  } else {
    *digitalPinToPortReg(modePins[RA][MODE2]) &= ~_BV(digitalPinToBit(modePins[RA][MODE2]));
    *digitalPinToPortReg(modePins[DC][MODE2]) &= ~_BV(digitalPinToBit(modePins[DC][MODE2]));
  }
  
  _delay_ms(1); //allow ic to reset
  *digitalPinToPortReg(resetPin[RA]) |= _BV(digitalPinToBit(resetPin[RA]));
  *digitalPinToPortReg(resetPin[DC]) |= _BV(digitalPinToBit(resetPin[DC])); 
  
  //ST4 pins to input with pullup
  *digitalPinToDirectionReg(st4Pin[RA][0]) &= ~_BV(digitalPinToBit(st4Pin[RA][0]));
  *digitalPinToPortReg(st4Pin[RA][0]) |= _BV(digitalPinToBit(st4Pin[RA][0]));
  *digitalPinToDirectionReg(st4Pin[RA][1]) &= ~_BV(digitalPinToBit(st4Pin[RA][1]));
  *digitalPinToPortReg(st4Pin[RA][1]) |= _BV(digitalPinToBit(st4Pin[RA][1]));
  *digitalPinToDirectionReg(st4Pin[DC][0]) &= ~_BV(digitalPinToBit(st4Pin[DC][0]));
  *digitalPinToPortReg(st4Pin[DC][0]) |= _BV(digitalPinToBit(st4Pin[DC][0])); 
  *digitalPinToDirectionReg(st4Pin[DC][1]) &= ~_BV(digitalPinToBit(st4Pin[DC][1]));
  *digitalPinToPortReg(st4Pin[DC][1]) |= _BV(digitalPinToBit(st4Pin[DC][1])); 
      
  // start Serial port:
  Serial_initialise(9600); //SyncScan runs at 9600Baud, use a serial port of your choice 
  while(Serial_available()){
    Serial_read(); //Clear the buffer
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
  char recievedChar = 0;
  signed char decoded = 0;
  for(;;){ //loop
    //static unsigned long lastMillis = millis();
    //static boolean isLedOn = false;
    char decodedPacket[11]; //temporary store for completed command ready to be processed
    //*digitalPinToPortReg(statusPin) &= ~_BV(digitalPinToBit(statusPin));
    if ((decoded == -2) || Serial_available()) { //is there a byte in buffer
      *digitalPinToPortReg(statusPin) ^= _BV(digitalPinToBit(statusPin)); //Toggle on the LED to indicate activity.
      if (decoded != -2) {
        recievedChar = Serial_read(); //get the next character in buffer
      } //else try to parse the last character again.
      decoded = synta_recieveCommand(decodedPacket,recievedChar); //once full command packet recieved, decodedPacket returns either error packet, or valid packet
      if (decoded > 0){ //Valid Packet
        decodeCommand(synta_command(),decodedPacket); //decode the valid packet
      } else if (decoded < 0){ //Error
        Serial_writeStr(decodedPacket); //send the error packet (recieveCommand() already generated the error packet, so just need to send it)
      } //otherwise command not yet fully recieved, so wait for next byte
    }
    
    //Check both axes - loop unravelled for speed efficiency - lots of Flash available.
    if(readyToGo[RA]==1){
      //if motor is stopped, then we can accelerate to target speed.
      //Otherwise don't start the next movement until we have stopped.
      if((cmd.stopped[RA])){
        signed char GVal = cmd.GVal[RA];
        if (canJumpToHighspeed){
          const byte* state;
          if ((GVal == 0) || (GVal == 3)) {
            state = modeState[STATE2];
            cmd_updateStepDir(RA,8);
          } else {
            state = modeState[STATE16];
            cmd_updateStepDir(RA,1);
          }
          if (state[0]) {
            *digitalPinToPortReg(modePins[RA][0]) |= _BV(digitalPinToBit(modePins[RA][0]));
          } else {
            *digitalPinToPortReg(modePins[RA][0]) &= ~_BV(digitalPinToBit(modePins[RA][0]));
          }
          if (state[1]) {
            *digitalPinToPortReg(modePins[RA][1]) |= _BV(digitalPinToBit(modePins[RA][1]));
          } else {
            *digitalPinToPortReg(modePins[RA][1]) &= ~_BV(digitalPinToBit(modePins[RA][1]));
          }
          if (state[2]) {
            *digitalPinToPortReg(modePins[RA][2]) |= _BV(digitalPinToBit(modePins[RA][2]));
          } else {
            *digitalPinToPortReg(modePins[RA][2]) &= ~_BV(digitalPinToBit(modePins[RA][2]));
          }
        } else {
          cmd_updateStepDir(RA,1);
        }
        if(GVal & 1){
          //This is the funtion that enables a slew type move.
          slewMode(RA); //Slew type
          readyToGo[RA] = 2;
        } else {
          //This is the function for goto mode. You may need to customise it for a different motor driver
          gotoMode(RA); //Goto Mode
          readyToGo[RA] = 0;
        }
      }
    }
    if(readyToGo[DC]==1){
      //if motor is stopped, then we can accelerate to target speed.
      //Otherwise don't start the next movement until we have stopped.
      if((cmd.stopped[DC])){
        signed char GVal = cmd.GVal[DC];
        if (canJumpToHighspeed){ //for those using microstepping, disable microstepping for high speed slews
          const byte* state;
          if ((GVal == 0) || (GVal == 3)) {
            state = modeState[STATE2];
            cmd_updateStepDir(DC,8);
          } else {
            state = modeState[STATE16];
            cmd_updateStepDir(DC,1);
          }
          if (state[0]) {
            *digitalPinToPortReg(modePins[DC][0]) |= _BV(digitalPinToBit(modePins[DC][0]));
          } else {
            *digitalPinToPortReg(modePins[DC][0]) &= ~_BV(digitalPinToBit(modePins[DC][0]));
          }
          if (state[1]) {
            *digitalPinToPortReg(modePins[DC][1]) |= _BV(digitalPinToBit(modePins[DC][1]));
          } else {
            *digitalPinToPortReg(modePins[DC][1]) &= ~_BV(digitalPinToBit(modePins[DC][1]));
          }
          if (state[2]) {
            *digitalPinToPortReg(modePins[DC][2]) |= _BV(digitalPinToBit(modePins[DC][2]));
          } else {
            *digitalPinToPortReg(modePins[DC][2]) &= ~_BV(digitalPinToBit(modePins[DC][2]));
          }
        } else {
          cmd_updateStepDir(DC,1);
        }
        if(GVal & 1){
          //This is the funtion that enables a slew type move.
          slewMode(DC); //Slew type
          readyToGo[DC] = 2;
        } else {
          //This is the function for goto mode. You may need to customise it for a different motor driver
          gotoMode(DC); //Goto Mode
          readyToGo[DC] = 0;
        }
      }
    }
  }
}

void decodeCommand(char command, char* packetIn){ //each command is axis specific. The axis being modified can be retrieved by calling synta_axis()
  char response[11]; //generated response string
  unsigned long responseData = 0; //data for response
  byte axis = synta_axis();
  unsigned int correction;
  //byte scalar = scalar[axis]+1;
  uint8_t oldSREG;
  switch(command){
    case 'e': //readonly, return the eVal (version number)
        responseData = cmd.eVal[axis]; //response to the e command is stored in the eVal function for that axis.
        break;
    case 'a': //readonly, return the aVal (steps per axis)
        responseData = cmd.aVal[axis]; //response to the a command is stored in the aVal function for that axis.
        break;
    case 'b': //readonly, return the bVal (sidereal step rate)
        responseData = cmd.bVal[axis]; //response to the b command is stored in the bVal function for that axis.
        if (progMode == 0){
          correction = (cmd.siderealIVal[axis] << 1);
          responseData = (responseData * (correction+1))/correction; //account for rounding inside Skywatcher DLL.
        }
        break;
    case 'g': //readonly, return the gVal (high speed multiplier)
        responseData = cmd.gVal[axis]; //response to the g command is stored in the gVal function for that axis.
        break;
    case 's': //readonly, return the sVal (steps per worm rotation)
        responseData = cmd.sVal[axis]; //response to the s command is stored in the sVal function for that axis.
        break;
    case 'f': //readonly, return the fVal (axis status)
        responseData = cmd_fVal(axis); //response to the f command is stored in the fVal function for that axis.
        break;
    case 'j': //readonly, return the jVal (current position)
        oldSREG = SREG; 
        cli();  //The next bit needs to be atomic, just in case the motors are running
        responseData = cmd.jVal[axis]; //response to the j command is stored in the jVal function for that axis.
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
        /*if (packetIn[0] == '0'){
          packetIn[0] = '2'; //don't allow a high torque goto. But do allow a high torque slew.
        }*/
        cmd_setGVal(axis, (packetIn[0] - '0')); //Store the current mode for the axis
        cmd_setDir(axis, (packetIn[1] - '0')); //Store the current direction for that axis
        readyToGo[axis] = 0;
        break;
    case 'H': //set goto position, return empty response (this sets the number of steps to move from cuurent position if in goto mode)
        cmd_setHVal(axis, synta_hexToLong(packetIn)); //set the goto position container (convert string to long first)
        readyToGo[axis] = 0;
        break;
    case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
        responseData = synta_hexToLong(packetIn);
        //if (responseData > minSpeed[axis]){
        //  responseData = minSpeed[axis]; //minimum speed limit <- if it is slower than this, there will be no acelleration.
        //}
        if (responseData == 0){
          responseData = 1; //limit IVal to minimum of 1.
        }
        cmd_setIVal(axis, responseData); //set the speed container (convert string to long first) 
        responseData = 0;
        if (readyToGo[axis] == 2){
          motorStart(axis,cmd.stepDir[axis]); //Update Speed.
        } else {
          readyToGo[axis] = 0;
        }
        break;
    case 'E': //set the current position, return empty response
        oldSREG = SREG; 
        cli();  //The next bit needs to be atomic, just in case the motors are running
        cmd_setjVal(axis, synta_hexToLong(packetIn)); //set the current position (used to sync to what EQMOD thinks is the current position at startup
        SREG = oldSREG;
        break;
    case 'F': //Enable the motor driver, return empty response
        if (progMode == 0){ //only allow motors to be enabled outside of programming mode.
          motorEnable(axis); //This enables the motors - gives the motor driver board power
        } else {
          command = 0; //force sending of error packet!.
        }
        break;
        
        
    //The following are used for configuration ----------
    case 'A': //store the aVal (steps per axis)
        cmd_setaVal(axis, synta_hexToLong(packetIn)); //store aVal for that axis.
        break;
    case 'B': //store the bVal (sidereal rate)
        cmd_setbVal(axis, synta_hexToLong(packetIn)); //store bVal for that axis.
        break;
    case 'S': //store the sVal (steps per worm rotation)
        cmd_setsVal(axis, synta_hexToLong(packetIn)); //store sVal for that axis.
        break;
    case 'n': //return the IVal (EQMOD Speed at sidereal)
        responseData = cmd.siderealIVal[axis];
        break;
    case 'N': //store the IVal (EQMOD Speed at sidereal)
        cmd_setsideIVal(axis, synta_hexToLong(packetIn)); //store sVal for that axis.
        break;
    case 'd': //return the driver version or step mode
        if (axis) {
          responseData = microstepConf; 
        } else {
          responseData = driverVersion;
        }
        break;
    case 'D': //store the driver verison and step modes
        if (axis) {
          microstepConf = synta_hexToByte(packetIn); //store step mode.
          canJumpToHighspeed = microstepConf > 4;
        } else {
          driverVersion = synta_hexToByte(packetIn); //store driver version.
        }
        break;
    case 'z': //return the Goto speed
        responseData = normalGotoSpeed[axis];
        break;
    case 'Z': //return the Goto speed factor
        normalGotoSpeed[axis] = synta_hexToByte(packetIn); //store the goto speed factor
        break;
    case 'c': //return the axisDirectionReverse
        responseData = encodeDirection[axis];
        break;
    case 'C': //store the axisDirectionReverse
        encodeDirection[axis] = packetIn[0] - '0'; //store sVal for that axis.
        break;
    case 'O': //set the programming mode.
        progMode = packetIn[0] - '0';              //MODES:  0 = Normal Ops (EQMOD). 1 = Validate EEPROM. 2 = Store to EEPROM. 3 = Rebuild EEPROM
        if (progMode != 0){
          motorStop(RA,1); //emergency axis stop.
          motorDisable(RA); //shutdown driver power.
          motorStop(DC,1); //emergency axis stop.
          motorDisable(DC); //shutdown driver power.
          readyToGo[RA] = 0;
          readyToGo[DC] = 0;
        }
        break;
    case 'T': //set mode, return empty response
        if(progMode & 2){
          //proceed with eeprom write
          if (progMode & 1) {
            buildEEPROM();
          } else {
            storeEEPROM();
          }
        } else if (progMode & 1){
          if(!checkEEPROM()){ //check if EEPROM contains valid data.
            command = 0; //force sending of an error packet.
          }
        }
        break;
    //---------------------------------------------------
    default: //Return empty response (deals with commands that don't do anything before the response sent (i.e 'J', 'R'), or do nothing at all (e.g. 'M', 'L') )
        break;
  }
  
  synta_assembleResponse(response, command, responseData); //generate correct response (this is required as is)
  Serial_writeStr(response); //send response to the serial port
  
  if ((command == 'R') && (response[0] == '=')){ //reset the uC
      wdt_enable(WDTO_15MS);
      exit(0);
  }
  if((command == 'J') && (progMode == 0)){ //J tells us we are ready to begin the requested movement. So signal we are ready to go and when the last movement complets this one will execute.
    readyToGo[axis] = 1;
    if (!(cmd.GVal[axis] & 1)){
      cmd_setGotoEn(axis,1);
    }
  }
}



void calculateRate(byte motor){
  
  unsigned long rate;
  unsigned long remainder;
  float floatRemainder;
  unsigned long divisor = cmd.bVal[motor];
  byte distWidth = DecimalDistnWidth;
  
  //When dividing a very large number by a much smaller on, float accuracy is abismal. So firstly we use integer math to split the division into quotient and remainder
  rate = timerCountRate / divisor; //Calculate the quotient
  remainder = timerCountRate % divisor; //Calculate the remainder
  
  //Then convert the remainder into a decimal number (division of a small number by a larger one, improving accuracy)
  floatRemainder = (float)remainder/(float)divisor; //Convert the remainder to a decimal.
    
  //Multiply the remainder by distributionWidth to work out an approximate number of extra clocks needed per full step (each step is 'distributionWidth' microsteps)
  floatRemainder *= (float)distWidth; 
  //This many extra cycles are needed:
  remainder = (unsigned long)(floatRemainder+0.5f); 
  
  //Now truncate to an unsigned int with a sensible max value (the int is to avoid register issues with the 16 bit timer)
  if(((Inter)rate).highByter.integer){
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
    
  byte x = cmd.siderealIVal[motor]>>3;
  byte y = ((unsigned int)rate)>>3;
  unsigned int divisior = x * y;
  cmd.stepRepeat[motor] = (byte)((18750+(divisior>>1))/divisior)-1; //note that we are rounding to neareast so we add half the divisor.
}

void motorEnable(byte axis){
  if (axis == RA){
    *digitalPinToPortReg(enablePin[RA]) &= ~_BV(digitalPinToBit(enablePin[RA])); //IC enabled
  } else {
    *digitalPinToPortReg(enablePin[DC]) &= ~_BV(digitalPinToBit(enablePin[DC])); //IC enabled
  }
  cmd_setFVal(axis,1);
  configureTimer(); //setup the motor pulse timers.
}

void motorDisable(byte axis){
  if (axis == RA){
    *digitalPinToPortReg(enablePin[RA]) |= _BV(digitalPinToBit(enablePin[RA])); //IC disabled
  } else {
    *digitalPinToPortReg(enablePin[DC]) |= _BV(digitalPinToBit(enablePin[DC])); //IC disabled
  }
  cmd_setFVal(axis,0);
}

void slewMode(byte axis){
  motorStart(axis,cmd.stepDir[axis]); //Begin PWM
}

void gotoMode(byte axis){
  unsigned int decelerationLength = gotoDecelerationLength[axis];
  byte dirMagnitude = abs(cmd.stepDir[axis]);
  char sign = cmd.dir[axis] ? -1 : 1;
  if (axis == RA){
    if (cmd.HVal[axis] < (decelerationLength>>1)){
      cmd_setHVal(axis,(decelerationLength>>1)); //fudge factor to account for star movement during decelleration.
    }
  } else {
    if (cmd.HVal[axis] < 16){
      cmd_setHVal(axis,16);
    }
  }
  decelerationLength = decelerationLength * dirMagnitude;
  //decelleration length is here a multiple of stepDir.
  unsigned long HVal = cmd.HVal[axis];
  unsigned long halfHVal = (HVal >> 1) + 1; //make deceleration slightly longer than acceleration
  unsigned int gotoSpeed = normalGotoSpeed[axis];
  if(dirMagnitude == 8){
    HVal &= 0xFFFFFFF8; //clear the lower bits to avoid overshoot.
  }
  if(dirMagnitude == 8){
    halfHVal &= 0xFFFFFFF8; //clear the lower bits to avoid overshoot.
  }
  //HVal and halfHVal are here a multiple of stepDir
  if (halfHVal < decelerationLength) {
    decelerationLength = halfHVal;
  }
  HVal -= decelerationLength;
  /*if (axis == RA){
    HVal += (decelerationLength>>2);
  }*/
  gotoPosn[axis] = cmd.jVal[axis] + (sign * HVal); //current position + relative change - decelleration region
  
  cmd_setIVal(axis, gotoSpeed);
  //calculateRate(axis);
  gotoCompleteRegister |= gotoCompleteBitMask(axis);
  decelerationLength = sign * decelerationLength;
  motorStart(axis,decelerationLength); //Begin PWM
  gotoRunning[axis] = 1; //start the goto.
}

inline void timerEnable(byte motor) {
  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn1));//00x
  timerPrescalarRegister(motor) |= (1<<CSn0);//xx1
}

inline void timerDisable(byte motor) {
  interruptControlRegister(motor) &= ~interruptControlBitMask(motor); //Disable timer interrupt
  timerPrescalarRegister(motor) &= ~((1<<CSn2) | (1<<CSn1) | (1<<CSn0));//00x
}

//As there is plenty of FLASH left, then to improve speed, I have created two motorStart functions (one for RA and one for DEC)
void motorStart(byte motor, signed int gotoDeceleration){
  if (motor == RA) {
    motorStartRA(gotoDeceleration);
  } else {
    motorStartDC(gotoDeceleration);
  }
}
void motorStartRA(signed int gotoDeceleration){
  unsigned int IVal = cmd.IVal[RA];
  unsigned int currentIVal;
  interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
  currentIVal = currentMotorSpeed(RA);
  interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
  unsigned int startSpeed;
  unsigned int stoppingSpeed;
  if (IVal > minSpeed[RA]){
    stoppingSpeed = IVal;
  } else {
    stoppingSpeed = minSpeed[RA];
  }
  if(cmd.stopped[RA]) {
    startSpeed = stoppingSpeed;
  } else {
    if (currentIVal < minSpeed[RA]) {
      startSpeed = currentIVal;
    } else {
      startSpeed = stoppingSpeed;
    }
  }
 
  interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
  cmd.currentIVal[RA] = cmd.IVal[RA];
  currentMotorSpeed(RA) = startSpeed;
  stopSpeed[RA] = stoppingSpeed;
  interruptCount(RA) = 1;
  if (encodeDirection[RA]^cmd.dir[RA]) {
    *digitalPinToPortReg(dirPin[RA]) |= _BV(digitalPinToBit(dirPin[RA]));
  } else {
    *digitalPinToPortReg(dirPin[RA]) &= ~_BV(digitalPinToBit(dirPin[RA]));
  }
  if(cmd.stopped[RA]) { //if stopped, configure timers
    stepIncrementRepeat[RA] = 0;
    distributionSegment(RA) = 0;
    int* decelerationSteps = (int*)&decelerationStepsLow(RA); //low and high are in sequential registers so we can treat them as an int in the sram.
    *decelerationSteps = gotoDeceleration;
    timerCountRegister(RA) = 0;
    interruptOVFCount(RA) = timerOVF[RA][0];
    timerEnable(RA);
    cmd_setStopped(RA, 0);
  }
  interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
}

void motorStartDC(signed int gotoDeceleration){
  unsigned int IVal = cmd.IVal[DC];
  unsigned int currentIVal;
  interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
  currentIVal = currentMotorSpeed(DC);
  interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt

  unsigned int startSpeed;
  unsigned int stoppingSpeed;
  if (IVal > minSpeed[DC]){
    stoppingSpeed = IVal;
  } else {
    stoppingSpeed = minSpeed[DC];
  }
  if(cmd.stopped[DC]) {
    startSpeed = stoppingSpeed;
  } else {
    if (currentIVal < minSpeed[DC]) {
      startSpeed = currentIVal;
    } else {
      startSpeed = stoppingSpeed;
    }
  }
 
  interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
  cmd.currentIVal[DC] = cmd.IVal[DC];
  currentMotorSpeed(DC) = startSpeed;
  stopSpeed[DC] = stoppingSpeed;
  interruptCount(DC) = 1;
  if (encodeDirection[DC]^cmd.dir[DC]) {
    *digitalPinToPortReg(dirPin[DC]) |= _BV(digitalPinToBit(dirPin[DC]));
  } else {
    *digitalPinToPortReg(dirPin[DC]) &= ~_BV(digitalPinToBit(dirPin[DC]));
  }
  if(cmd.stopped[DC]) { //if stopped, configure timers
    stepIncrementRepeat[DC] = 0;
    distributionSegment(DC) = 0;
    int* decelerationSteps = (int*)&decelerationStepsLow(DC); //low and high are in sequential registers so we can treat them as an int in the sram.
    *decelerationSteps = gotoDeceleration;
    timerCountRegister(DC) = 0;
    interruptOVFCount(DC) = timerOVF[DC][0];
    timerEnable(DC);
    cmd_setStopped(DC, 0);
  }
  interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt
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
    cmd_setGotoEn(RA,0); //Not in goto mode.
    cmd_setStopped(RA,1); //mark as stopped
    readyToGo[RA] = 0;
    gotoRunning[RA] = 0;
  } else if (!cmd.stopped[RA]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
    //trigger ISR based decelleration
    //readyToGo[RA] = 0;
    cmd_setGotoEn(RA,0); //No longer in goto mode.
    gotoRunning[RA] = 0;
    interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
    if(cmd.currentIVal[RA] < minSpeed[RA]){
      if(stopSpeed[RA] > minSpeed[RA]){
        stopSpeed[RA] = minSpeed[RA];
      }
    }/* else {
      stopSpeed[RA] = cmd.currentIVal[RA];
    }*/
    cmd.currentIVal[RA] = stopSpeed[RA] + 1;//cmd.stepIncrement[motor];
    interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
  }
}

void motorStopDC(byte emergency){
  if (emergency) {
    //trigger instant shutdown of the motor in an emergency.
    timerDisable(DC);
    cmd_setGotoEn(DC,0); //Not in goto mode.
    cmd_setStopped(DC,1); //mark as stopped
    readyToGo[DC] = 0;
    gotoRunning[DC] = 0;
  } else if (!cmd.stopped[DC]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
    //trigger ISR based decelleration
    //readyToGo[motor] = 0;
    cmd_setGotoEn(DC,0); //No longer in goto mode.
    gotoRunning[DC] = 0;
    interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
    if(cmd.currentIVal[DC] < minSpeed[DC]){
      if(stopSpeed[DC] > minSpeed[DC]){
        stopSpeed[DC] = minSpeed[DC];
      }
    }/* else {
      stopSpeed[DC] = cmd.currentIVal[DC];
    }*/
    cmd.currentIVal[DC] = stopSpeed[DC] + 1;//cmd.stepIncrement[motor];
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
  if(!cmd.gotoEn[RA]){
    //Only allow when not it goto mode.
    stopped = cmd.stopped[RA];
    unsigned int newSpeed = cmd.siderealIVal[RA];
    if (cmd.dir[RA] && !stopped) {
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
      cmd.currentIVal[RA] = newSpeed;
      if (stopped) {
        cmd.dir[RA] = 0; //set direction
        register byte one asm("r22");
        asm volatile(
          "ldi %0,0x01  \n\t"
          : "=r" (one)
          : "0" (one)
          :
        );
        cmd.stepDir[RA] = one; //set step direction
        cmd.GVal[RA] = one; //slew mode
        //calculateRate(RA);
        motorStart(RA,one);
      } else {
        if (stopSpeed[RA] < cmd.currentIVal[RA]) {
          stopSpeed[RA] = cmd.currentIVal[RA]; //ensure that RA doesn't stop.
        }
      }
    } else {
ignoreRAST4:
      cmd.currentIVal[RA] = newSpeed;
    }
  }
  if(!cmd.gotoEn[DC]){
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
      cmd.stepDir[DC] = stepDir; //set step direction
      cmd.dir[DC] = dir; //set direction
      cmd.currentIVal[DC] = (cmd.siderealIVal[DC] << 2); //move at 0.25x sidereal rate
      cmd.GVal[DC] = 1; //slew mode
      //calculateRate(DC);
      motorStart(DC,1);
    } else {
      cmd.currentIVal[DC] = stopSpeed[DC] + 1;//make our target >stopSpeed so that ISRs bring us to a halt.
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
    byte index = ((DecimalDistnWidth-1) << 1) & timeSegment;
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
      unsigned long jVal = cmd.jVal[DC];
      jVal = jVal + cmd.stepDir[DC];
      cmd.jVal[DC] = jVal;
      if(gotoRunning[DC]){
        if (gotoPosn[DC] == jVal){ //reached the decelleration marker. Note that this line loads gotoPosn[] into r24:r27
          //will decellerate the rest of the way. So first move gotoPosn[] to end point.
          if (gotoCompleteRegister & gotoCompleteBitMask(DC)) {
            /*
            unsigned long gotoPos = gotoPosn[DC];
            if (cmd.stepDir[DC] < 0){
              gotoPosn[DC] = gotoPos - decelerationSteps(DC);
            } else {
              gotoPosn[DC] = gotoPos + decelerationSteps(DC);
            }
            */
            //--- This is a heavily optimised version of the code commented out just above ------------
            //During compare of jVal and gotoPosn[], gotoPosn[] was loaded into r24 to r27
            //register char stepDir asm("r21") = cmd.stepDir[DC];
            register unsigned long newGotoPosn asm("r18");
            asm volatile(
              "in   %A0, %1   \n\t" //read in the number of decelleration steps
              "in   %B0, %2   \n\t" //which is either negative or positive
              "ldi  %C0, 0xFF \n\t" //assume it is and sign extend
              "sbrs %B0, 7    \n\t" //if negative, then skip
              "eor  %C0, %C0  \n\t" //else clear the sign extension
              "mov  %D0, %C0  \n\t" //complete any sign extension as necessary
              "add  %A0, r24  \n\t" //and then add on to get the final stopping position
              "adc  %B0, r25  \n\t"
              "adc  %C0, r26  \n\t"
              "adc  %D0, r27  \n\t"
              : "=a" (newGotoPosn) //goto selects r18:r21. This adds sts commands for all 4 bytes
              : /*"r" (stepDir),*/"I" (_SFR_IO_ADDR(decelerationStepsLow(DC))),"I" (_SFR_IO_ADDR(decelerationStepsHigh(DC)))       //stepDir is in r19 to match second byte of goto.
              :
            );
            gotoPosn[DC] = newGotoPosn;
            //-----------------------------------------------------------------------------------------
            gotoCompleteRegister &= ~gotoCompleteBitMask(DC); //say we are stopping
            cmd.currentIVal[DC] = stopSpeed[DC]; //decellerate to min speed.
          } else {
            goto stopMotorISR3;
          }
        }
      }
      if (currentMotorSpeed(DC) > stopSpeed[DC]) {
stopMotorISR3:
        if(gotoRunning[DC]){ //if we are currently running a goto then 
          cmd.gotoEn[DC] = 0; //Goto mode complete
          gotoRunning[DC] = 0; //Goto no longer running
        } //otherwise don't as it cancels a 'goto ready' state
        cmd.stopped[DC] = 1; //mark as stopped
        timerDisable(DC);
      }
      asm volatile (
        "pop  r26       \n\t"
        "pop  r27       \n\t"
      );
    } else {
      stepPort(DC) = port | stepHigh(DC);
      register unsigned int iVal asm("r20") = cmd.currentIVal[DC];
      if (iVal != startVal){
        port = stepIncrementRepeat[DC];
        register byte repeat asm("r19") = cmd.stepRepeat[DC];
        if (repeat == port){
          register byte increment asm("r0");// = cmd.stepIncrement[DC];
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
    byte index = ((DecimalDistnWidth-1) << 1) & timeSegment;
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
      unsigned long jVal = cmd.jVal[RA];
      jVal = jVal + cmd.stepDir[RA];
      cmd.jVal[RA] = jVal;
      if(gotoRunning[RA]){
        if (jVal == gotoPosn[RA]){ //reached the decelleration marker
          //will decellerate the rest of the way. So first move gotoPosn[] to end point.
          if (gotoCompleteRegister & gotoCompleteBitMask(RA)) {
            /*
            unsigned long gotoPos = gotoPosn[RA];
            if (cmd.stepDir[RA] < 0){
              gotoPosn[RA] = gotoPos + decelerationSteps(RA);
            } else {
              gotoPosn[RA] = gotoPos - decelerationSteps(RA);
            }
            */
            //--- This is a heavily optimised version of the code commented out just above ------------
            //register char stepDir asm("r21") = cmd.stepDir[RA];
            register unsigned long newGotoPosn asm("r18");
            asm volatile(
              "in   %A0, %1   \n\t"
              "in   %B0, %2   \n\t"
              "ldi  %C0, 0xFF \n\t"
              //"cp   %1, __zero_reg__  \n\t"
              "sbrs %B0, 7    \n\t" //if negative, then skip
              "eor  %C0, %C0  \n\t"
              "mov  %D0, %C0  \n\t"
              "add  %A0, r24  \n\t"
              "adc  %B0, r25  \n\t"
              "adc  %C0, r26  \n\t"
              "adc  %D0, r27  \n\t"
              : "=a" (newGotoPosn) //goto selects r18:r21. This adds sts commands for all 4 bytes
              : /*"r" (stepDir),*/"I" (_SFR_IO_ADDR(decelerationStepsLow(RA))), "I" (_SFR_IO_ADDR(decelerationStepsHigh(RA)))       //temp is in r19 to match second byte of goto.
              :
            );
            gotoPosn[RA] = newGotoPosn;
            gotoCompleteRegister &= ~gotoCompleteBitMask(RA); //say we are stopping
            cmd.currentIVal[RA] = stopSpeed[RA]; //decellerate to min speed. This is possible in at most 80 steps.
          } else {
            goto stopMotorISR1;
          }
        }
      }
      if (currentMotorSpeed(RA) > stopSpeed[RA]) {
stopMotorISR1:
        if(gotoRunning[RA]){ //if we are currently running a goto then 
          cmd.gotoEn[RA] = 0; //Goto mode complete
          gotoRunning[RA] = 0; //Goto complete.
        } //otherwise don't as it cancels a 'goto ready' state
        cmd.stopped[RA] = 1; //mark as stopped
        timerDisable(RA);
      }
      asm volatile (
        "pop  r26       \n\t"
        "pop  r27       \n\t"
      );
    } else {
      stepPort(RA) = port | stepHigh(RA);
      register unsigned int iVal asm("r20") = cmd.currentIVal[RA];
      if (iVal != startVal){
        port = stepIncrementRepeat[RA];
        register byte repeat asm("r19") = cmd.stepRepeat[RA];
        if (repeat == port){
          register byte increment asm("r0");// = cmd.stepIncrement[RA];
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

