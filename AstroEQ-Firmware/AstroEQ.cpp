/*
  Code written by Thomas Carpenter 2012
  
  With thanks Chris over at the EQMOD Yahoo group for explaining the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Synta
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts, and also a great many custom mount configurations.
 
  Current Verison: 7.5
*/

//Only works with ATmega162, and Arduino Mega boards (1280 and 2560)
#if defined(__AVR_ATmega162__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)


/*
 * Include headers
 */
 
#include "AstroEQ.h" //AstroEQ header

#include "EEPROMReader.h" //Read config file
#include "SerialLink.h" //Serial Port
#include "UnionHelpers.h" //Union prototypes
#include "synta.h" //Synta Communications Protocol.
#include <util/delay.h>    
#include <avr/wdt.h>




/*
 * Global Variables
 */
byte stepIncrement[2];
byte readyToGo[2] = {0,0};
unsigned long gotoPosn[2] = {0UL,0UL}; //where to slew to
bool encodeDirection[2];
byte progMode = RUNMODE; //MODES:  0 = Normal Ops (EQMOD). 1 = Validate EEPROM. 2 = Store to EEPROM. 3 = Rebuild EEPROM
byte microstepConf;
byte driverVersion;

#define timerCountRate 8000000
#define DecimalDistnWidth 32
unsigned int timerOVF[2][DecimalDistnWidth];
bool canJumpToHighspeed = false;
byte stepIncrementRepeat[2] = {0,0};
unsigned int gotoDecelerationLength[2];




/*
 * Helper Macros
 */
#define distributionSegment(m) (m ? GPIOR1 : GPIOR0)
#define currentMotorSpeed(m) (m ? OCR3A : OCR3B)
#define interruptCount(m) (m ? OCR1A : OCR1B)
#define interruptOVFCount(m) (m ? ICR3 : ICR1)
#define interruptControlRegister(m) (m ? TIMSK3 : TIMSK1)
#define interruptControlBitMask(m) (m ? _BV(ICIE3) : _BV(ICIE1))
#define timerCountRegister(m) (m ? TCNT3 : TCNT1)
#define timerPrescalarRegister(m) (m ? TCCR3B : TCCR1B)
#define gotoDeceleratingBitMask(m) (m ? _BV(3) : _BV(2))
#define gotoRunningBitMask(m) (m ? _BV(1) : _BV(0))
#define gotoControlRegister GPIOR2




/*
 * Inline functions
 */
inline bool gotoRunning(const byte axis) {
    return (gotoControlRegister & gotoRunningBitMask(axis));
}
inline bool gotoDecelerating(const byte axis) {
    return (gotoControlRegister & gotoDeceleratingBitMask(axis));
}
inline void setGotoRunning(const byte axis) {
    gotoControlRegister |= gotoRunningBitMask(axis);
}
inline void clearGotoRunning(const byte axis) {
    gotoControlRegister &= ~gotoRunningBitMask(axis);
}
inline void setGotoDecelerating(const byte axis) {
    gotoControlRegister |= gotoDeceleratingBitMask(axis);
}
inline void clearGotoDecelerating(const byte axis) {
    gotoControlRegister &= ~gotoDeceleratingBitMask(axis);
}




/*
 * Declare constant arrays of pin numbers. Also generate macros for fast port writes.
 */
const byte standalonePin[2] = {gpioPin_1_Define,gpioPin_2_Define};
const byte statusPin = statusPin_Define;
const byte resetPin[2] = {resetPin_0_Define,resetPin_1_Define};
const byte dirPin[2] = {dirPin_0_Define,dirPin_1_Define};
const byte enablePin[2] = {enablePin_0_Define,enablePin_1_Define};
const byte stepPin[2] = {stepPin_0_Define,stepPin_1_Define};
const byte st4Pin[2][2] = {{ST4AddPin_0_Define,ST4SubPin_0_Define},{ST4AddPin_1_Define,ST4SubPin_1_Define}};
const byte modePins[2][3] = {{modePins0_0_Define,modePins1_0_Define,modePins2_0_Define},{modePins0_1_Define,modePins1_1_Define,modePins2_1_Define}};

#define setPinDir(p,d) {if(d){*digitalPinToDirectionReg((p)) |= _BV(digitalPinToBit((p)));}else{*digitalPinToDirectionReg((p)) &= ~_BV(digitalPinToBit((p)));}}
#define setPinValue(p,v) {if(v){*digitalPinToPortReg((p)) |= _BV(digitalPinToBit((p)));}else{*digitalPinToPortReg((p)) &= ~_BV(digitalPinToBit((p)));}}
#define getPinValue(p) (!!(*digitalPinToPinReg((p)) & _BV(digitalPinToBit((p)))))
#define togglePin(p) {*digitalPinToPortReg((p)) ^= _BV(digitalPinToBit((p)));}



/*
 * Generate Mode Mappings
 */

#define MODE0 0
#define MODE1 1
#define MODE2 2
#define MODE0DIR 3
#define MODE1DIR 4
#define MODE2DIR 5
#define SPEEDNORM 0
#define SPEEDFAST 1
byte modeState[2] = {((HIGH << MODE2) | (HIGH << MODE1) | (HIGH << MODE0)), (( LOW << MODE2) | ( LOW << MODE1) | (HIGH << MODE0))}; //Default to A498x with 1/16th stepping

void buildModeMapping(byte microsteps, byte driverVersion){
    //For microstep modes less than 8, we cannot jump to high speed, so we use the SPEEDFAST mode maps. Given that the SPEEDFAST maps are generated for the microstepping modes >=8
    //anyway, we can simply multiply the number of microsteps by 8 if it is less than 8 and thus reduce the number of cases in the mode generation switch statement below 
    if (microsteps < 8){
        microsteps *= 8;
    }
    //Generate the mode mapping for the current driver version and microstepping modes.
    switch (microsteps) {
        case 8:
            // 1/8
            modeState[SPEEDNORM] = (driverVersion == DRV8834) ? (( LOW << MODE2) | (HIGH << MODE1) | (  LOW << MODE0)) : (( LOW << MODE2) | (HIGH << MODE1) | (HIGH << MODE0));
            // 1/1
            modeState[SPEEDFAST] =                                                                                       (( LOW << MODE2) | ( LOW << MODE1) | ( LOW << MODE0));
            break;
        case 32:
            // 1/32
            modeState[SPEEDNORM] = (driverVersion == DRV8834) ? (( LOW << MODE2) | (HIGH << MODE1) | (FLOAT << MODE0)) : ((HIGH << MODE2) | (HIGH << MODE1) | (HIGH << MODE0));
            // 1/4
            modeState[SPEEDFAST] = (driverVersion == DRV8834) ? (( LOW << MODE2) | ( LOW << MODE1) | (FLOAT << MODE0)) : (( LOW << MODE2) | (HIGH << MODE1) | ( LOW << MODE0));
            break;
        case 16:
        default:  //Unknown. Default to half/sixteenth stepping
            // 1/16
            modeState[SPEEDNORM] = (driverVersion == DRV882x) ? ((HIGH << MODE2) | ( LOW << MODE1) | (  LOW << MODE0)) : ((HIGH << MODE2) | (HIGH << MODE1) | (HIGH << MODE0));
            // 1/2
            modeState[SPEEDFAST] =                                                                                       (( LOW << MODE2) | ( LOW << MODE1) | (HIGH << MODE0));
            break;
    }
}




/*
 * System Initialisation Routines
 */

unsigned int calculateDecelerationLength (byte axis){
    unsigned int currentSpeed = cmd.normalGotoSpeed[axis];
    unsigned int stoppingSpeed = cmd.minSpeed[axis];
    unsigned int stepRepeat = cmd.stepRepeat[axis] + 1;
    unsigned int numberOfSteps = 0;
    while(currentSpeed < stoppingSpeed) {
        currentSpeed += (currentSpeed >> 5) + 1; //work through the decelleration formula.
        numberOfSteps += stepRepeat; //n more steps
    }
    //number of steps now contains how many steps required to slow to a stop.
    return numberOfSteps;
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
    if((unsigned int)(rate >> 16)){
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

void systemInitialiser(){    
    
    encodeDirection[RA] = EEPROM_readByte(RAReverse_Address) ? CMD_REVERSE : CMD_FORWARD;  //reverse the right ascension if 1
    encodeDirection[DC] = EEPROM_readByte(DECReverse_Address) ? CMD_REVERSE : CMD_FORWARD; //reverse the declination if 1
    
    driverVersion = EEPROM_readByte(Driver_Address);
    microstepConf = EEPROM_readByte(Microstep_Address);
    
    canJumpToHighspeed = (microstepConf >= 8); //Gear change is enabled if the microstep mode can change by a factor of 8.
    
    synta_initialise(1281,(canJumpToHighspeed ? 8 : 1)); //initialise mount instance, specify version!

    buildModeMapping(microstepConf, driverVersion);
    
    if(!checkEEPROM()){
        progMode = PROGMODE; //prevent AstroEQ startup if EEPROM is blank.
    }
    calculateRate(RA); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
    calculateRate(DC); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
    gotoDecelerationLength[RA] = calculateDecelerationLength(RA);
    gotoDecelerationLength[DC] = calculateDecelerationLength(DC);
}




/*
 * EEPROM Validation and Programming Routines
 */

bool checkEEPROM(){
    char temp[9] = {0};
    EEPROM_readString(temp,8,AstroEQID_Address);
    if(strncmp(temp,"AstroEQ",8)){
        return false;
    }
    if (driverVersion > DRV8834){
        return false; //invalid value.
    }
    if ((driverVersion == A498x) && microstepConf > 16){
        return false; //invalid value.
    } else if (microstepConf > 32){
        return false; //invalid value.
    }
    if ((cmd.siderealIVal[RA] > 1200) || (cmd.siderealIVal[RA] < MIN_IVAL)) {
        return false; //invalid value.
    }
    if ((cmd.siderealIVal[DC] > 1200) || (cmd.siderealIVal[DC] < MIN_IVAL)) {
        return false; //invalid value.
    }
    if(cmd.normalGotoSpeed[RA] == 0){
        return false; //invalid value.
    }
    if(cmd.normalGotoSpeed[DC] == 0){
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
    EEPROM_writeByte(cmd.normalGotoSpeed[RA],RAGoto_Address);
    EEPROM_writeByte(cmd.normalGotoSpeed[DC],DECGoto_Address);
    EEPROM_writeInt(cmd.siderealIVal[RA],IVal1_Address);
    EEPROM_writeInt(cmd.siderealIVal[DC],IVal2_Address);
}




/*
 * AstroEQ firmware main() function
 */

int main(void) {
    //Enable global interrupt flag
    sei();
    //Initialise global variables from the EEPROM
    systemInitialiser();
    //Disable Timer 2
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

    //Status pin to output
    setPinDir  (statusPin,OUTPUT);

    //Standalone pin to input pullup
    setPinDir  (standalonePin[0],INPUT);
    setPinValue(standalonePin[0], HIGH);
    setPinDir  (standalonePin[1],INPUT);
    setPinValue(standalonePin[1], HIGH);
    
    //Reset pins to output
    setPinDir  (resetPin[RA],OUTPUT);
    setPinValue(resetPin[RA],   LOW);  //Motor driver in Reset
    setPinDir  (resetPin[DC],OUTPUT);
    setPinValue(resetPin[DC],   LOW);  //Motor driver in Reset 
    
    //Enable pins to output
    setPinDir  (enablePin[RA],OUTPUT);
    setPinValue(enablePin[RA],  HIGH); //Motor Driver Disabled
    setPinDir  (enablePin[DC],OUTPUT);
    setPinValue(enablePin[DC],  HIGH); //Motor Driver Disabled
    
    //Step pins to output
    setPinDir  (stepPin[RA],OUTPUT);
    setPinValue(stepPin[RA],   LOW);
    setPinDir  (stepPin[DC],OUTPUT);
    setPinValue(stepPin[DC],   LOW);
    
    //Direction pins to output
    setPinDir  (dirPin[RA],OUTPUT);
    setPinValue(dirPin[RA],   LOW);
    setPinDir  (dirPin[DC],OUTPUT);
    setPinValue(dirPin[DC],   LOW);
    
    //Load the correct mode
    {
        byte state = modeState[canJumpToHighspeed ? SPEEDNORM : SPEEDFAST]; //Extract the current default mode - if we can change speed, then we start at normal speed, otherwise we start at full speed
    
        setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0   )));
        setPinDir  (modePins[RA][MODE0],!(state & (byte)(1<<MODE0DIR))); //For the DRV8834 type, we also need to set the direction of the Mode0 bit to be an input if floating is required for this step mode.
        setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0   )));
        setPinDir  (modePins[DC][MODE0],!(state & (byte)(1<<MODE0DIR))); //For the DRV8834 type, we also need to set the direction of the Mode0 bit to be an input if floating is required for this step mode.
        setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1   )));
        setPinDir  (modePins[RA][MODE1],  OUTPUT                      );
        setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1   )));
        setPinDir  (modePins[DC][MODE1],  OUTPUT                      );
        setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2   )));
        setPinDir  (modePins[RA][MODE2],  OUTPUT                      );
        setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2   )));
        setPinDir  (modePins[DC][MODE2],  OUTPUT                      );
    }

    //Give some time for the Motor Drivers to reset.
    _delay_ms(1);

    //Then bring them out of reset.
    setPinValue(resetPin[RA],HIGH);
    setPinValue(resetPin[DC],HIGH);
    
    //ST4 pins to input with pullup
    setPinDir  (st4Pin[RA][0],OUTPUT);
    setPinValue(st4Pin[RA][0],HIGH  );
    setPinDir  (st4Pin[RA][1],OUTPUT);
    setPinValue(st4Pin[RA][1],HIGH  );
    setPinDir  (st4Pin[DC][0],OUTPUT);
    setPinValue(st4Pin[DC][0],HIGH  );
    setPinDir  (st4Pin[DC][1],OUTPUT);
    setPinValue(st4Pin[DC][1],HIGH  );
        
    //Initialise the Serial port:
    Serial_initialise(9600); //SyncScan runs at 9600Baud, use a serial port of your choice as defined in SerialLink.h
    
  
    //Configure interrupt for ST4 connection
#if defined(__AVR_ATmega162__)
    //For ATMega162:
    PCMSK0 =  0xF0; //PCINT[4..7]
    PCICR &= ~_BV(PCIE1); //disable PCInt[8..15] vector
    PCICR |=  _BV(PCIE0); //enable  PCInt[0..7]  vector
#elif defined(ALTERNATE_ST4)
    //For ATMega1280/2560 with Alterante ST4 pins
    PCMSK2 =  0x0F; //PCINT[16..23]
    PCICR |=  _BV(PCIE2); //enable  PCInt[16..23] vector
    PCICR &= ~_BV(PCIE1); //disable PCInt[8..15]  vector
    PCICR &= ~_BV(PCIE0); //disable PCInt[0..7]   vector
#else
    //For ATMega1280/2560 with Standard ST4 pins
    PCMSK0 =  0x0F; //PCINT[0..3]
    PCICR &= ~_BV(PCIE2); //disable PCInt[16..23] vector
    PCICR &= ~_BV(PCIE1); //disable PCInt[8..15]  vector
    PCICR |=  _BV(PCIE0); //enable  PCInt[0..7]   vector
#endif
    char recievedChar = 0;
    signed char decoded = 0;
    char decodedPacket[11]; //temporary store for completed command ready to be processed
    bool standaloneMode = false;
    bool highspeedMode = false;
    for(;;){ //Run loop

        if ((getPinValue(standalonePin[0]) && getPinValue(standalonePin[1])) || progMode) { //EQMOD Mode or Programming Mode

            if (standaloneMode) {
                //If we have just moved from standalone to EQMOD mode,
                standaloneMode = false; //clear the standalone flag
                Commands_configureST4Speed(CMD_ST4_DEFAULT); //And reset the ST4 speeds
            }
            
            //Check if we need to run the command parser
            
            if ((decoded == -2) || Serial_available()) { //is there a byte in buffer or we still need to process the previous byte?
                //Toggle on the LED to indicate activity.
                togglePin(statusPin);
                //See what character we need to parse
                if (decoded != -2) {
                    //get the next character in buffer
                    recievedChar = Serial_read(); 
                } //otherwise we will try to parse the previous character again.
                //Append the current character and try to parse the command
                decoded = synta_recieveCommand(decodedPacket,recievedChar); 
                //Once full command packet recieved, synta_recieveCommand populates either an error packet (and returns -1), or data packet (returns 1). If incomplete, decodedPacket is unchanged and 0 is returned
                if (decoded > 0){ //Valid Packet
                    decodeCommand(synta_command(),decodedPacket); //decode the valid packet
                } else if (decoded < 0){ //Error
                    Serial_writeStr(decodedPacket); //send the error packet (recieveCommand() already generated the error packet, so just need to send it)
                } //otherwise command not yet fully recieved, so wait for next byte
            }

        } else { //Stand-alone mode

            standaloneMode = true; //We are in standalone mode.
            //See if we need to enable the motors in stand-alone mode
            
            if(!cmd.FVal[RA] || !cmd.FVal[DC]) {
                //If the axes aren't energised, then we enable the motors and configure the mount
                byte oldSREG = SREG; 
                cli();  //The next bit needs to be atomic
                cmd_setjVal(RA, 0x800000); //set the current position to the middle
                cmd_setjVal(DC, 0x800000); //set the current position to the middle
                SREG = oldSREG;
                byte state = modeState[canJumpToHighspeed ? SPEEDNORM : SPEEDFAST]; //Extract the default mode - if we can change speed, then we use normal speed, otherwise we use full speed
                setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0   )));
                setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0   )));
                setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1   )));
                setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1   )));
                setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2   )));
                setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2   )));
                cmd_updateStepDir(RA,1);
                cmd_updateStepDir(DC,1);
                motorEnable(RA);
                motorEnable(DC);
            }

            //Then parse the stand-alone mode speed
            if (getPinValue(standalonePin[1])) {
                //If in normal speed mode
                if (highspeedMode) {
                    //But we have just changed from highspeed
                    Commands_configureST4Speed(CMD_ST4_STANDALONE); //Change the ST4 speeds
                    highspeedMode = false;
                }
            } else {
                //If in high speed mode
                if (!highspeedMode) {
                    //But we have just changed from normal speed
                    Commands_configureST4Speed(CMD_ST4_HIGHSPEED); //Change the ST4 speeds
                    highspeedMode = true;
                }
            }
        }
    
    
        //Check both axes - loop unravelled for speed efficiency - lots of Flash available.
        if(readyToGo[RA]==1){
            //If we are ready to begin a movement which requires the motors to be reconfigured
            if((cmd.stopped[RA])){
                //Once the motor is stopped, we can accelerate to target speed.
                signed char GVal = cmd.GVal[RA];
                if (canJumpToHighspeed){
                    //If we are allowed to enable high speed, see if we need to
                    byte state;
                    if ((GVal == 0) || (GVal == 3)) {
                        //If a high speed mode command
                        state = modeState[SPEEDFAST]; //Select the high speed mode
                        cmd_updateStepDir(RA,cmd.gVal[RA]);
                    } else {
                        state = modeState[SPEEDNORM];
                        cmd_updateStepDir(RA,1);
                    }
                    setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0)));
                    setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1)));
                    setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2)));
                } else {
                    //Otherwise we never need to change the speed
                    cmd_updateStepDir(RA,1); //Just move along at one step per step
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
            } //Otherwise don't start the next movement until we have stopped.
        }
        if(readyToGo[DC]==1){
            //If we are ready to begin a movement which requires the motors to be reconfigured
            if((cmd.stopped[DC])){
                //Once the motor is stopped, we can accelerate to target speed.
                signed char GVal = cmd.GVal[DC];
                if (canJumpToHighspeed){
                    //If we are allowed to enable high speed, see if we need to
                    byte state;
                    if ((GVal == 0) || (GVal == 3)) {
                        //If a high speed mode command
                        state = modeState[SPEEDFAST]; //Select the high speed mode
                        cmd_updateStepDir(DC,cmd.gVal[DC]);
                    } else {
                        state = modeState[SPEEDNORM];
                        cmd_updateStepDir(DC,1);
                    }
                    setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0)));
                    setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1)));
                    setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2)));
                } else {
                    //Otherwise we never need to change the speed
                    cmd_updateStepDir(DC,1); //Just move along at one step per step
                }
                if(GVal & 1){
                    //This is the funtion that enables a slew type move.
                    slewMode(DC); //Slew type
                    readyToGo[DC] = 2; //We are now in a running mode which speed can be changed without stopping motor (unless a command changes the direction)
                } else {
                    //This is the function for goto mode.
                    gotoMode(DC); //Goto Mode
                    readyToGo[DC] = 0; //We are now in a mode where no further changes can be made to the motor (apart from requesting a stop) until the go-to movement is done.
                }
            } //Otherwise don't start the next movement until we have stopped.
        }
    }//End of run loop
}




/*
 * Decode and Perform the Command
 */

void decodeCommand(char command, char* packetIn){ //each command is axis specific. The axis being modified can be retrieved by calling synta_axis()
    char response[11]; //generated response string
    unsigned long responseData = 0; //data for response
    byte axis = synta_axis();
    unsigned int correction;
    byte oldSREG;
    switch(command){
        case 'e': //readonly, return the eVal (version number)
            responseData = cmd.eVal[axis]; //response to the e command is stored in the eVal function for that axis.
            break;
        case 'a': //readonly, return the aVal (steps per axis)
            responseData = cmd.aVal[axis]; //response to the a command is stored in the aVal function for that axis.
            break;
        case 'b': //readonly, return the bVal (sidereal step rate)
            responseData = cmd.bVal[axis]; //response to the b command is stored in the bVal function for that axis.
            if (!progMode){
                //If not in programming mode, we need to apply a correction factor to ensure that calculations in EQMOD round correctly
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
            cmd_setDir(axis, (packetIn[1] != '0') ? CMD_REVERSE : CMD_FORWARD); //Store the current direction for that axis
            readyToGo[axis] = 0;
            break;
        case 'H': //set goto position, return empty response (this sets the number of steps to move from cuurent position if in goto mode)
            cmd_setHVal(axis, synta_hexToLong(packetIn)); //set the goto position container (convert string to long first)
            readyToGo[axis] = 0;
            break;
        case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
            responseData = synta_hexToLong(packetIn);
            if (responseData == 0){
                //We cannot handle a speed of infinity!
                responseData = 1; //limit IVal to minimum of 1.
            }
            cmd_setIVal(axis, responseData); //set the speed container (convert string to long first) 
            responseData = 0;
            if (readyToGo[axis] == 2){
                //If we are in a running mode which allows speed update without motor reconfiguration
                motorStart(axis); //Simply update the speed.
            } else {
                //Otherwise we are no longer ready to go until the next :J command is received
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
                canJumpToHighspeed = (microstepConf >=8);
            } else {
                driverVersion = synta_hexToByte(packetIn); //store driver version.
            }
            break;
        case 'z': //return the Goto speed
            responseData = cmd.normalGotoSpeed[axis];
            break;
        case 'Z': //return the Goto speed factor
            cmd.normalGotoSpeed[axis] = synta_hexToByte(packetIn); //store the goto speed factor
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
    if((command == 'J') && (progMode == 0)){ //J tells us we are ready to begin the requested movement.
        readyToGo[axis] = 1; //So signal we are ready to go and when the last movement complets this one will execute.
        if (!(cmd.GVal[axis] & 1)){
            //If go-to mode requested
            cmd_setGotoEn(axis,CMD_ENABLED);
        }
    }
}




void motorEnable(byte axis){
    if (axis == RA){
        setPinValue(enablePin[RA],LOW); //IC enabled
        cmd_setFVal(RA,CMD_ENABLED);
    } else {
        setPinValue(enablePin[DC],LOW); //IC enabled
        cmd_setFVal(DC,CMD_ENABLED);
    }
    configureTimer(); //setup the motor pulse timers.
}

void motorDisable(byte axis){
    if (axis == RA){
        setPinValue(enablePin[RA],HIGH); //IC enabled
        cmd_setFVal(RA,CMD_DISABLED);
    } else {
        setPinValue(enablePin[DC],HIGH); //IC enabled
        cmd_setFVal(DC,CMD_DISABLED);
    }
}

void slewMode(byte axis){
    motorStart(axis); //Begin PWM
}

void gotoMode(byte axis){
    unsigned int decelerationLength = gotoDecelerationLength[axis];
    byte dirMagnitude = abs(cmd.stepDir[axis]);
    byte dir = cmd.dir[axis];
    if (axis == RA){
        if (cmd.HVal[RA] < (decelerationLength>>1)){
            cmd_setHVal(RA,(decelerationLength>>1)); //fudge factor to account for star movement during decelleration.
        }
    } else {
        if (cmd.HVal[DC] < 16){
            cmd_setHVal(DC,16);
        }
    }
    decelerationLength = decelerationLength * dirMagnitude;
    //decelleration length is here a multiple of stepDir.
    unsigned long HVal = cmd.HVal[axis];
    unsigned long halfHVal = (HVal >> 1) + 1; //make deceleration slightly longer than acceleration
    unsigned int gotoSpeed = cmd.normalGotoSpeed[axis];
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
    gotoPosn[axis] = cmd.jVal[axis] + ((dir == CMD_REVERSE) ? -HVal : HVal); //current position + relative change - decelleration region
    
    cmd_setIVal(axis, gotoSpeed);
    clearGotoDecelerating(axis);
    setGotoRunning(axis); //start the goto.
    motorStart(axis); //Begin PWM
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
void motorStart(byte motor){
    if (motor == RA) {
        motorStartRA();
    } else {
        motorStartDC();
    }
}
void motorStartRA(){
    unsigned int IVal = cmd.IVal[RA];
    unsigned int currentIVal;
    unsigned int startSpeed;
    unsigned int stoppingSpeed;
    
    interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
    currentIVal = currentMotorSpeed(RA);
    interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
    
    if (IVal > cmd.minSpeed[RA]){
        stoppingSpeed = IVal;
    } else {
        stoppingSpeed = cmd.minSpeed[RA];
    }
    if(cmd.stopped[RA]) {
        startSpeed = stoppingSpeed;
    } else if (currentIVal < cmd.minSpeed[RA]) {
        startSpeed = currentIVal;
    } else {
        startSpeed = stoppingSpeed;
    }
    
    interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
    cmd.currentIVal[RA] = cmd.IVal[RA];
    currentMotorSpeed(RA) = startSpeed;
    cmd.stopSpeed[RA] = stoppingSpeed;
    interruptCount(RA) = 1;
    setPinValue(dirPin[RA],(encodeDirection[RA] != cmd.dir[RA]));
    
    if(cmd.stopped[RA]) { //if stopped, configure timers
        stepIncrementRepeat[RA] = 0;
        distributionSegment(RA) = 0;
        timerCountRegister(RA) = 0;
        interruptOVFCount(RA) = timerOVF[RA][0];
        timerEnable(RA);
        cmd_setStopped(RA, CMD_RUNNING);
    }
    interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
}

void motorStartDC(){
    unsigned int IVal = cmd.IVal[DC];
    unsigned int currentIVal;
    interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
    currentIVal = currentMotorSpeed(DC);
    interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt
    
    unsigned int startSpeed;
    unsigned int stoppingSpeed;
    if (IVal > cmd.minSpeed[DC]){
        stoppingSpeed = IVal;
    } else {
        stoppingSpeed = cmd.minSpeed[DC];
    }
    if(cmd.stopped[DC]) {
        startSpeed = stoppingSpeed;
    } else if (currentIVal < cmd.minSpeed[DC]) {
        startSpeed = currentIVal;
    } else {
        startSpeed = stoppingSpeed;
    }
    
    interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
    cmd.currentIVal[DC] = cmd.IVal[DC];
    currentMotorSpeed(DC) = startSpeed;
    cmd.stopSpeed[DC] = stoppingSpeed;
    interruptCount(DC) = 1;
    setPinValue(dirPin[DC],(encodeDirection[DC] != cmd.dir[DC]));
    
    if(cmd.stopped[DC]) { //if stopped, configure timers
        stepIncrementRepeat[DC] = 0;
        distributionSegment(DC) = 0;
        timerCountRegister(DC) = 0;
        interruptOVFCount(DC) = timerOVF[DC][0];
        timerEnable(DC);
        cmd_setStopped(DC, CMD_RUNNING);
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
        cmd_setGotoEn(RA,CMD_DISABLED); //Not in goto mode.
        cmd_setStopped(RA,CMD_STOPPED); //mark as stopped
        readyToGo[RA] = 0;
        clearGotoRunning(RA);
    } else if (!cmd.stopped[RA]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
        //trigger ISR based decelleration
        //readyToGo[RA] = 0;
        cmd_setGotoEn(RA,CMD_DISABLED); //No longer in goto mode.
        clearGotoRunning(RA);
        interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
        if(cmd.currentIVal[RA] < cmd.minSpeed[RA]){
            if(cmd.stopSpeed[RA] > cmd.minSpeed[RA]){
                cmd.stopSpeed[RA] = cmd.minSpeed[RA];
            }
        }/* else {
            stopSpeed[RA] = cmd.currentIVal[RA];
        }*/
        cmd.currentIVal[RA] = cmd.stopSpeed[RA] + 1;//cmd.stepIncrement[motor];
        interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
    }
}

void motorStopDC(byte emergency){
    if (emergency) {
        //trigger instant shutdown of the motor in an emergency.
        timerDisable(DC);
        cmd_setGotoEn(DC,CMD_DISABLED); //Not in goto mode.
        cmd_setStopped(DC,CMD_STOPPED); //mark as stopped
        readyToGo[DC] = 0;
        clearGotoRunning(DC);
    } else if (!cmd.stopped[DC]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
        //trigger ISR based decelleration
        //readyToGo[motor] = 0;
        cmd_setGotoEn(DC,CMD_DISABLED); //No longer in goto mode.
        clearGotoRunning(DC);
        interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
        if(cmd.currentIVal[DC] < cmd.minSpeed[DC]){
            if(cmd.stopSpeed[DC] > cmd.minSpeed[DC]){
                cmd.stopSpeed[DC] = cmd.minSpeed[DC];
            }
        }/* else {
        stopSpeed[DC] = cmd.currentIVal[DC];
        }*/
        cmd.currentIVal[DC] = cmd.stopSpeed[DC] + 1;//cmd.stepIncrement[motor];
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

#ifdef ALTERNATE_ST4
ISR(PCINT2_vect)
#else
ISR(PCINT0_vect)
#endif
{
    //ST4 Pin Change Interrupt Handler.
    byte dir;
    byte stepDir;
    if(!cmd.gotoEn[RA]){
        //Only allow when not it goto mode.
        bool stopped = (cmd.stopped[RA] == CMD_STOPPED) || (cmd.st4RAReverse == CMD_REVERSE);
        unsigned int newSpeed;
        if (cmd.dir[RA] && !stopped) {
            goto ignoreRAST4; //if travelling in the wrong direction and not allowing reverse, then ignore.
        }
        if (!getPinValue(st4Pin[RA][1])) {
            //RA-
            if (cmd.st4RAReverse == CMD_REVERSE) {
                dir = CMD_REVERSE;
                stepDir = -1;
            } else {
                dir = CMD_FORWARD;
                stepDir = 1;
            }
            newSpeed = cmd.st4RAIVal[1]; //------------ 0.75x sidereal rate
            goto setRASpeed;
        } else if (!getPinValue(st4Pin[RA][0])) {
            //RA+
            dir = CMD_FORWARD;
            stepDir = 1;
            newSpeed = cmd.st4RAIVal[0]; //------------ 1.25x sidereal rate
setRASpeed:
            cmd.currentIVal[RA] = newSpeed;
            if (stopped) {
                cmd.stepDir[RA] = stepDir; //set step direction
                cmd.dir[RA] = dir; //set direction
                cmd.GVal[RA] = 1; //slew mode
                motorStartRA();
            } else if (cmd.stopSpeed[RA] < cmd.currentIVal[RA]) {
                cmd.stopSpeed[RA] = cmd.currentIVal[RA]; //ensure that RA doesn't stop.
            }
        } else {
ignoreRAST4:
            dir = CMD_FORWARD;
            stepDir = 1;
            newSpeed = cmd.siderealIVal[RA];
            goto setRASpeed;
        }
    }
    if(!cmd.gotoEn[DC]){
        //Only allow when not it goto mode.
        if (!getPinValue(st4Pin[DC][1])) {
            //DEC-
            dir = CMD_REVERSE;
            stepDir = -1;
            goto setDECSpeed;
        } else if (!getPinValue(st4Pin[DC][0])) {
            //DEC+
            dir = CMD_FORWARD;
            stepDir = 1;
setDECSpeed:
            cmd.stepDir[DC] = stepDir; //set step direction
            cmd.dir[DC] = dir; //set direction
            cmd.currentIVal[DC] = cmd.st4DecIVal; //move at 0.25x sidereal rate
            cmd.GVal[DC] = 1; //slew mode
            motorStartDC();
        } else {
            cmd.currentIVal[DC] = cmd.stopSpeed[DC] + 1;//make our target >stopSpeed so that ISRs bring us to a halt.
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
        
        register unsigned int startVal asm("r24") = currentMotorSpeed(DC);
        //byte port = STEP0PORT;
        //unsigned int startVal = currentMotorSpeed(DC);
        interruptCount(DC) = startVal; //start val is used for specifying how many interrupts between steps. This tends to IVal after acceleration
        if (getPinValue(stepPin[DC])){
            asm volatile (
            "push r27       \n\t"
            "push r26       \n\t"
            );
            setPinValue(stepPin[DC],LOW);
            unsigned long jVal = cmd.jVal[DC];
            jVal = jVal + cmd.stepDir[DC];
            cmd.jVal[DC] = jVal;
            if(gotoRunning(DC) && !gotoDecelerating(DC)){
                if (gotoPosn[DC] == jVal){ //reached the decelleration marker. Note that this line loads gotoPosn[] into r24:r27
                    //will decellerate the rest of the way. So first move gotoPosn[] to end point.
                    //if (!gotoDecelerating(DC)) {
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
                    //  register unsigned long newGotoPosn asm("r18");
                    //  asm volatile(
                    //    "in   %A0, %1   \n\t" //read in the number of decelleration steps
                    //    "in   %B0, %2   \n\t" //which is either negative or positive
                    //    "ldi  %C0, 0xFF \n\t" //assume it is and sign extend
                    //    "sbrs %B0, 7    \n\t" //if negative, then skip
                    //    "eor  %C0, %C0  \n\t" //else clear the sign extension
                    //    "mov  %D0, %C0  \n\t" //complete any sign extension as necessary
                    //    "add  %A0, r24  \n\t" //and then add on to get the final stopping position
                    //    "adc  %B0, r25  \n\t"
                    //    "adc  %C0, r26  \n\t"
                    //    "adc  %D0, r27  \n\t"
                    //    : "=a" (newGotoPosn) //goto selects r18:r21. This adds sts commands for all 4 bytes
                    //    : /*"r" (stepDir),*/"I" (_SFR_IO_ADDR(decelerationStepsLow(DC))),"I" (_SFR_IO_ADDR(decelerationStepsHigh(DC)))       //stepDir is in r19 to match second byte of goto.
                    //    :
                    //  );
                    //  gotoPosn[DC] = newGotoPosn;
                    //-----------------------------------------------------------------------------------------
                    setGotoDecelerating(DC); //say we are stopping
                    cmd.currentIVal[DC] = cmd.stopSpeed[DC]+1; //decellerate to below the stop speed.
                    //} else {
                    //  goto stopMotorISR3;
                    //}
                }
            }
            if (currentMotorSpeed(DC) > cmd.stopSpeed[DC]) {
                //stopMotorISR3:
                if(gotoRunning(DC)){ //if we are currently running a goto then 
                    cmd_setGotoEn(DC,CMD_DISABLED); //Goto mode complete
                    clearGotoRunning(DC); //Goto complete.
                } //otherwise don't as it cancels a 'goto ready' state
                cmd_setStopped(DC,CMD_STOPPED); //mark as stopped
                timerDisable(DC);
            }
            asm volatile (
                "pop  r26       \n\t"
                "pop  r27       \n\t"
            );
        } else {
            setPinValue(stepPin[DC],HIGH);
            register unsigned int iVal asm("r20") = cmd.currentIVal[DC];
            if (iVal != startVal){
                register byte stepIncrRpt asm("r18") = stepIncrementRepeat[DC];
                register byte repeat asm("r19") = cmd.stepRepeat[DC];
                if (repeat == stepIncrRpt){
                    register byte increment asm("r0");// = cmd.stepIncrement[DC];
                    /*
                    stepIncrement[DC] = max(255,(startVal >> 5) + 1);
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
                        "movw %0 , %1    \n\t"  //copy iVal to startVal
                        "1:              \n\t"
                        : "=&w" (startVal) //startVal is in r24:25
                        : "a" (iVal), "t" (increment) //iVal is in r20:21
                        : 
                    );
                    currentMotorSpeed(DC) = startVal; //store startVal
                    stepIncrRpt = 0;
                } else {
                    /*
                    stepIncrementRepeat[DC]++;
                    */
                    stepIncrRpt++;
                }
                stepIncrementRepeat[DC] = stepIncrRpt;
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
        
        register unsigned int startVal asm("r24") = currentMotorSpeed(RA);
        //byte port = STEP1PORT;
        //unsigned int startVal = currentMotorSpeed(RA);
        interruptCount(RA) = startVal; //start val is used for specifying how many interrupts between steps. This tends to IVal after acceleration
        if (getPinValue(stepPin[RA])){
            asm volatile (
                "push r27       \n\t"
                "push r26       \n\t"
            );
            setPinValue(stepPin[RA],LOW);
            unsigned long jVal = cmd.jVal[RA];
            jVal = jVal + cmd.stepDir[RA];
            cmd.jVal[RA] = jVal;
            if(gotoRunning(RA) && !gotoDecelerating(RA)){
                if (gotoPosn[RA] == jVal){ //reached the decelleration marker
                    //will decellerate the rest of the way. So first move gotoPosn[] to end point.
                    //if (!gotoDecelerating(RA)) {
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
                    //  register unsigned long newGotoPosn asm("r18");
                    //  asm volatile(
                    //    "in   %A0, %1   \n\t"
                    //    "in   %B0, %2   \n\t"
                    //    "ldi  %C0, 0xFF \n\t"
                    //    //"cp   %1, __zero_reg__  \n\t"
                    //    "sbrs %B0, 7    \n\t" //if negative, then skip
                    //    "eor  %C0, %C0  \n\t"
                    //    "mov  %D0, %C0  \n\t"
                    //    "add  %A0, r24  \n\t"
                    //    "adc  %B0, r25  \n\t"
                    //    "adc  %C0, r26  \n\t"
                    //    "adc  %D0, r27  \n\t"
                    //    : "=a" (newGotoPosn) //goto selects r18:r21. This adds sts commands for all 4 bytes
                    //    : /*"r" (stepDir),*/"I" (_SFR_IO_ADDR(decelerationStepsLow(RA))), "I" (_SFR_IO_ADDR(decelerationStepsHigh(RA)))       //temp is in r19 to match second byte of goto.
                    //    :
                    //  );
                    //  gotoPosn[RA] = newGotoPosn;
                    //-----------------------------------------------------------------------------------------
                    setGotoDecelerating(RA); //say we are stopping
                    cmd.currentIVal[RA] = cmd.stopSpeed[RA]+1; //decellerate to below stop speed.
                    //} else {
                    //  goto stopMotorISR1;
                    //}
                }
            }
            if (currentMotorSpeed(RA) > cmd.stopSpeed[RA]) {
                //stopMotorISR1:
                if(gotoRunning(RA)){ //if we are currently running a goto then 
                    cmd_setGotoEn(RA,CMD_DISABLED); //Goto mode complete
                    clearGotoRunning(RA); //Goto complete.
                } //otherwise don't as it cancels a 'goto ready' state
                cmd_setStopped(RA,CMD_STOPPED); //mark as stopped
                timerDisable(RA);
            }
            asm volatile (
                "pop  r26       \n\t"
                "pop  r27       \n\t"
            );
        } else {
            setPinValue(stepPin[RA],HIGH);
            register unsigned int iVal asm("r20") = cmd.currentIVal[RA];
            if (iVal != startVal){
                register byte stepIncrRpt asm("r18") = stepIncrementRepeat[RA];
                register byte repeat asm("r19") = cmd.stepRepeat[RA];
                if (repeat == stepIncrRpt){
                    register byte increment asm("r0");// = cmd.stepIncrement[RA];
                    /*
                    stepIncrement[RA] = max(255,(startVal >> 5) + 1);
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
                        "movw %0 , r18   \n\t"
                        "add  %A0, %2    \n\t"
                        "adc  %B0, __zero_reg__    \n\t"
                        "cp   %A1, %A0   \n\t"
                        "cpc  %B1, %B0   \n\t"
                        "brge 1f         \n\t" //branch if (startVal+increment) <= iVal
                        "movw %0 , %1    \n\t"  //copy iVal to startVal
                        "1:              \n\t"
                        : "=&w" (startVal) //startVal is in r24:25
                        : "a" (iVal), "t" (increment) //iVal is in r20:21
                        : 
                    );
                    currentMotorSpeed(RA) = startVal; //store startVal
                    stepIncrRpt = 0;
                } else {
                /*
                stepIncrementRepeat++;
                */
                    stepIncrRpt++;
                }
                stepIncrementRepeat[RA] = stepIncrRpt;
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

