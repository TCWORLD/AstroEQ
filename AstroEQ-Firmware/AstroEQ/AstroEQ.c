/*
  Code written by Thomas Carpenter 2012-2017
  
  With thanks Chris over at the EQMOD Yahoo group for assisting decoding the Skywatcher protocol
  
  
  Equatorial mount tracking system for integration with EQMOD using the Skywatcher/Synta
  communication protocol.
 
  Works with EQ5, HEQ5, and EQ6 mounts, and also a great many custom mount configurations.
 
  Current Version: <see AstroEQ.h header>
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
#include <util/delay_basic.h>
#include <util/crc16.h>
#include <avr/wdt.h>


// Watchdog disable on boot.
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void)
{
    wdt_disable();
    return;
}

/*
 * Defines
 */

/*
 * Global Variables
 */
byte stepIncrement[2];
byte readyToGo[2] = {MOTION_START_NOTREADY, MOTION_START_NOTREADY};
unsigned long gotoPosn[2] = {0UL,0UL}; //where to slew to
bool encodeDirection[2];
byte progMode = RUNMODE; //MODES:  0 = Normal Ops (EQMOD). 1 = Validate EEPROM. 2 = Store to EEPROM. 3 = Rebuild EEPROM
byte progModeEntryCount = 0; //Must send 10 non-zero progMode commands to switch out of run-time. This is to prevent accidental entry by EQMOD.
byte microstepConf;
byte driverVersion;
bool standaloneMode = false; //Initially not in standalone mode (EQMOD mode)
bool syntaMode = true; //And Synta processing is enabled.

#define timerCountRate 8000000

#define DecimalDistnWidth 32
unsigned int timerOVF[2][DecimalDistnWidth];
bool canJumpToHighspeed = false;
bool defaultSpeedState = SPEEDNORM;
bool disableGearChange = false;
bool allowAdvancedHCDetection = false;
unsigned int gotoDecelerationLength[2];
byte accelTableRepeatsLeft[2] = {0,0};
byte accelTableIndex[2] = {0,0};

/*
 * Helper Macros
 */

//These should not be used directly. They are used for
//overloading of the register access macros to allow
//set and get operations to be performed using the same
//name.
#define GET_TERNARY_MACRO(m,_1,NAME,...) NAME
#define SET_TERNARY_REGISTER(rt,rf,m,c) if(m) {rt = c;} else {rf = c;}
#define GET_TERNARY_REGISTER(rt,rf,m)   ((m) ? rt : rf)
#define GET_SINGULAR_MACRO(_1,NAME,...) NAME
#define SET_SINGULAR_REGISTER(r,c) {r = c;}
#define GET_SINGULAR_REGISTER(r)   (r)

//Registers
//For setting an RA/DC register -> macro(RA,setValue)
//For getting an RA/DC register -> macro(DC)
#define distributionSegment(...)       GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )(GPIOR1,GPIOR2,__VA_ARGS__)
#define currentMotorSpeed(...)         GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )( OCR3A, OCR3B,__VA_ARGS__)
#define irqToNextStep(...)             GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )( OCR1A, OCR1B,__VA_ARGS__)
#define timerCountRegister(...)        GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )( TCNT3, TCNT1,__VA_ARGS__)
#define timerPrescalarRegister(...)    GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )(TCCR3B,TCCR1B,__VA_ARGS__)
#define interruptOVFCount(...)         GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )(  ICR3,  ICR1,__VA_ARGS__)
#define interruptControlRegister(...)  GET_TERNARY_MACRO(__VA_ARGS__,  SET_TERNARY_REGISTER ,  GET_TERNARY_REGISTER )(TIMSK3,TIMSK1,__VA_ARGS__)
#define polarscopeDutyRegister(...)   GET_SINGULAR_MACRO(__VA_ARGS__, SET_SINGULAR_REGISTER , GET_SINGULAR_REGISTER )(        OCR2A,__VA_ARGS__)
#define gotoControlRegister GPIOR0
//Bit Masks
#define interruptControlBitMask(m)  (m ? _BV(ICIE3) : _BV(ICIE1))
#define gotoDeceleratingBitMask(m)  (m ? _BV(    3) : _BV(    2))
#define gotoRunningBitMask(m)       (m ? _BV(    1) : _BV(    0))





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
inline bool motionIsSlew(const unsigned char GVal) {
    return !!(GVal & 1); // CMD_GVAL_HIGHSPEED_SLEW or CMD_GVAL_LOWSPEED_SLEW (Odd Nummbers)
}
inline bool motionIsGoto(const unsigned char GVal) {
    return !(GVal & 1); // CMD_GVAL_HIGHSPEED_GOTO or CMD_GVAL_LOWSPEED_GOTO (Even Numbers)
}
inline bool motionIsLowSpeed(const unsigned char GVal) {
    return ((GVal == CMD_GVAL_LOWSPEED_SLEW) || (GVal == CMD_GVAL_LOWSPEED_GOTO));
}

/*
 * Generate Mode Mappings
 */

#define MODE0 0
#define MODE1 1
#define MODE2 2
#define MODE0DIR 3
#define MODE1DIR 4
#define MODE2DIR 5
byte modeState[2] = {((LOW << MODE2) | (HIGH << MODE1) | (HIGH << MODE0)), (( LOW << MODE2) | ( LOW << MODE1) | (LOW << MODE0))}; //Default to 1/1 stepping as that is the same for all

void buildModeMapping(byte microsteps, byte driverVersion){
    //For microstep modes less than 8, we cannot jump to high speed, so we use the SPEEDFAST mode maps. Given that the SPEEDFAST maps are generated for the micro-stepping modes >=8
    //anyway, we can simply multiply the number of microsteps by 8 if it is less than 8 and thus reduce the number of cases in the mode generation switch statement below 
    if (microsteps < 8){
        microsteps *= 8;
    }
    //Generate the mode mapping for the current driver version and micro-stepping modes.
    switch (driverVersion) {
        case A498x:
            switch (microsteps) {
                case 8:
                    // 1/8
                    modeState[SPEEDNORM] = ((  LOW << MODE2) | ( HIGH << MODE1) | ( HIGH << MODE0));
                    // 1/1
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | (  LOW << MODE1) | (  LOW << MODE0));
                    break;
                case 32:
                    // 1/32 (unavailable)
                    modeState[SPEEDNORM] = ((  LOW << MODE2) | ( HIGH << MODE1) | (  LOW << MODE0));
                    // 1/4
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | ( HIGH << MODE1) | (  LOW << MODE0));
                    break;
                case 16:
                default:  //Unknown. Default to half/sixteenth stepping
                    // 1/16
                    modeState[SPEEDNORM] = (( HIGH << MODE2) | ( HIGH << MODE1) | ( HIGH << MODE0));
                    // 1/2
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | (  LOW << MODE1) | ( HIGH << MODE0));
                break;
            }
            break;
        case DRV882x:
            switch (microsteps) {
                case 8:
                    // 1/8
                    modeState[SPEEDNORM] = ((  LOW << MODE2) | ( HIGH << MODE1) | ( HIGH << MODE0));
                    // 1/1
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | (  LOW << MODE1) | (  LOW << MODE0));
                    break;
                case 32:
                    // 1/32
                    modeState[SPEEDNORM] = (( HIGH << MODE2) | ( HIGH << MODE1) | ( HIGH << MODE0));
                    // 1/4
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | ( HIGH << MODE1) | (  LOW << MODE0));
                    break;
                case 16:
                default:  //Unknown. Default to half/sixteenth stepping
                    // 1/16
                    modeState[SPEEDNORM] = ((  HIGH << MODE2) | (  LOW << MODE1) | (  LOW << MODE0));
                    // 1/2
                    modeState[SPEEDFAST] = ((   LOW << MODE2) | (  LOW << MODE1) | ( HIGH << MODE0));
                    break;
            }
            break;
        case DRV8834:
            switch (microsteps) {
                case 8:
                    // 1/8
                    modeState[SPEEDNORM] = ((  LOW << MODE2) | ( HIGH << MODE1) | (  LOW << MODE0));
                    // 1/1
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | (  LOW << MODE1) | (  LOW << MODE0));
                    break;
                case 32:
                    // 1/32
                    modeState[SPEEDNORM] = ((FLOAT << MODE2) | ( HIGH << MODE1) | (FLOAT << MODE0));
                    // 1/4
                    modeState[SPEEDFAST] = ((FLOAT << MODE2) | (  LOW << MODE1) | (FLOAT << MODE0));
                    break;
                case 16:
                default:  //Unknown. Default to half/sixteenth stepping
                    // 1/16
                    modeState[SPEEDNORM] = (( HIGH << MODE2) | ( HIGH << MODE1) | ( HIGH << MODE0));
                    // 1/2
                    modeState[SPEEDFAST] = (( HIGH << MODE2) | (  LOW << MODE1) | ( HIGH << MODE0));
                    break;
            }
            break;
        case TMC2100:
            switch (microsteps) {
                case 8:
                    // 1/8 (unavailable)
                    modeState[SPEEDNORM] = ((  LOW << MODE2) | (  LOW << MODE1) | (  LOW << MODE0));
                    // 1/1
                    modeState[SPEEDFAST] = ((  LOW << MODE2) | (  LOW << MODE1) | (  LOW << MODE0));
                    break;
                case 32:
                    // 1/32 (unavailable)
                    modeState[SPEEDNORM] = ((FLOAT << MODE2) | ( HIGH << MODE1) | (FLOAT << MODE0));
                    // 1/4
                    modeState[SPEEDFAST] = ((FLOAT << MODE2) | ( HIGH << MODE1) | (FLOAT << MODE0));
                    break;
                case 16:
                default:  //Unknown. Default to half/sixteenth stepping
                    // 1/16
                    modeState[SPEEDNORM] = ((  LOW << MODE2) | (FLOAT << MODE1) | (  LOW << MODE0));
                    // 1/2
                    modeState[SPEEDFAST] = ((FLOAT << MODE2) | (  LOW << MODE1) | (FLOAT << MODE0));
                    break;
        }
        break;
        default:  //Unknown. Default to half/sixteenth stepping
            // 1/16
            modeState[SPEEDNORM] = (( HIGH << MODE2) | ( HIGH << MODE1) | ( HIGH << MODE0));
            // 1/2
            modeState[SPEEDFAST] = ((  LOW << MODE2) | (  LOW << MODE1) | ( HIGH << MODE0));
            break;
    }
}




/*
 * System Initialisation Routines
 */

void calculateDecelerationLength (byte axis){

    unsigned int gotoSpeed = cmd.normalGotoSpeed[axis];
    byte lookupTableIndex = 0;
    unsigned int numberOfSteps = 0;
    //Work through the acceleration table until we get to the right speed (accel and decel are same number of steps)
    while(lookupTableIndex < AccelTableLength) {
        if (cmd.accelTable[axis][lookupTableIndex].speed <= gotoSpeed) {
            //If we have reached the element at which we are now at the right speed
            break; //We have calculated the number of accel steps and therefore number of decel steps.
        }
        numberOfSteps = numberOfSteps + cmd.accelTable[axis][lookupTableIndex].repeats + 1; //Add on the number of steps at this speed (1 step + number of repeats)
        lookupTableIndex++;
    }
    //number of steps now contains how many steps required to slow to a stop.
    gotoDecelerationLength[axis] = numberOfSteps;
}

void calculateRate(byte axis){
  
    unsigned long rate;
    unsigned long remainder;
    float floatRemainder;
    unsigned long divisor = cmd.bVal[axis];
    byte distWidth = DecimalDistnWidth;
    
    //When dividing a very large number by a much smaller on, float accuracy is abysmal. So firstly we use integer math to split the division into quotient and remainder
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
    rate--; //For some reason not doing this on the Arduino Mega works better. Possibly because of frequency drift on Arduino Mega oscillator.
#endif
  
    for (byte i = 0; i < distWidth; i++){
        timerOVF[axis][i] = rate;
    }
  
    //evenly distribute the required number of extra clocks over the full step.
    for (unsigned long i = 0; i < remainder; i++){
        float distn = i;
        distn *= (float)distWidth;
        distn /= (float)remainder;
        byte index = (byte)ceil(distn);
        timerOVF[axis][index] += 1;
    }
    
}

void systemInitialiser(){    
    
    encodeDirection[RA] = EEPROM_readByte(RAReverse_Address) ? CMD_REVERSE : CMD_FORWARD;  //reverse the right ascension if 1
    encodeDirection[DC] = EEPROM_readByte(DECReverse_Address) ? CMD_REVERSE : CMD_FORWARD; //reverse the declination if 1
    
    driverVersion = EEPROM_readByte(Driver_Address);
    microstepConf = EEPROM_readByte(Microstep_Address);

    allowAdvancedHCDetection = !EEPROM_readByte(AdvHCEnable_Address);
    
    defaultSpeedState = (microstepConf >= 8) ? SPEEDNORM : SPEEDFAST;
    disableGearChange = !EEPROM_readByte(GearEnable_Address);
    canJumpToHighspeed = (microstepConf >= 8) && !disableGearChange; //Gear change is enabled if the microstep mode can change by a factor of 8.
        
    synta_initialise(ASTROEQ_VER,(canJumpToHighspeed ? 8 : 1)); //initialise mount instance, specify version!
    
    buildModeMapping(microstepConf, driverVersion);
    
    if(!checkEEPROM(false)){
        progMode = PROGMODE; //prevent AstroEQ startup if EEPROM is blank.
    } else {
        calculateRate(RA); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
        calculateRate(DC); //Initialise the interrupt speed table. This now only has to be done once at the beginning.
        calculateDecelerationLength(RA);
        calculateDecelerationLength(DC);
    }    
    //Status pin to output low
    setPinDir  (statusPin,OUTPUT);
    setPinValue(statusPin, (progMode == PROGMODE) ? HIGH : LOW);
    #ifdef statusPinShadow_Define
    setPinDir  (statusPinShdw,OUTPUT);
    setPinValue(statusPinShdw, (progMode == PROGMODE) ? HIGH : LOW);
    #endif

    //Standalone Speed/IRQ pin to input no-pull-up
    setPinDir  (standalonePin[  STANDALONE_IRQ], INPUT);
    setPinValue(standalonePin[  STANDALONE_IRQ],  HIGH); //enable pull-up to pull IRQ high.

    //Standalone Pull-up/Pull-down pin to output high
    setPinDir  (standalonePin[ STANDALONE_PULL],OUTPUT);
    setPinValue(standalonePin[ STANDALONE_PULL],  HIGH);
    
    //ST4 pins to input with pull-up
    setPinDir  (st4Pins[RA][ST4P],INPUT);
    setPinValue(st4Pins[RA][ST4P],HIGH );
    setPinDir  (st4Pins[RA][ST4N],INPUT);
    setPinValue(st4Pins[RA][ST4N],HIGH );
    setPinDir  (st4Pins[DC][ST4P],INPUT);
    setPinValue(st4Pins[DC][ST4P],HIGH );
    setPinDir  (st4Pins[DC][ST4N],INPUT);
    setPinValue(st4Pins[DC][ST4N],HIGH );
    
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
    byte state = modeState[defaultSpeedState]; //Extract the default mode - If the microstep mode is >= then we start in NORMAL mode, otherwise we use FAST mode

    setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0   )));
    setPinDir  (modePins[RA][MODE0],!(state & (byte)(1<<MODE0DIR)));
    setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0   )));
    setPinDir  (modePins[DC][MODE0],!(state & (byte)(1<<MODE0DIR)));
    setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1   )));
    setPinDir  (modePins[RA][MODE1],!(state & (byte)(1<<MODE1DIR)));
    setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1   )));
    setPinDir  (modePins[DC][MODE1],!(state & (byte)(1<<MODE1DIR)));
    setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2   )));
    setPinDir  (modePins[RA][MODE2],!(state & (byte)(1<<MODE2DIR)));
    setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2   )));
    setPinDir  (modePins[DC][MODE2],!(state & (byte)(1<<MODE2DIR)));

    //Give some time for the Motor Drivers to reset.
    _delay_ms(1);

    //Then bring them out of reset.
    setPinValue(resetPin[RA],HIGH);
    setPinValue(resetPin[DC],HIGH);
    
#if defined(__AVR_ATmega162__)
    //Disable Timer 0
    //Timer 0 registers are being used as general purpose data storage for high efficiency
    //interrupt routines. So timer must be fully disabled. The ATMegaxxx0 has three of these
    //registers, but the ATMega162 doesn't, so I've had to improvise and use other registers
    //instead. See PinMappings.h for the ATMega162 to see which registers have been #defined
    //as GPIORx.
    TIMSK &= ~(_BV(TOIE0) | _BV(OCIE0));
    TCCR0 = 0;
#endif

    //Configure SNAP1 GPIO Pin
    setPinDir  (snapPin, OUTPUT);
    setPinValue(snapPin, LOW);

    //Configure Polar Scope LED Pin
    setPinDir  (pwmPin,OUTPUT);
    setPinValue(pwmPin,LOW);

    //Configure Polar Scope PWM Generator (Timer 2)
    ASSR = 0;
    polarscopeDutyRegister(0); //Start as off
    TCCR2A = 0;
    TCCR2B = 0;
    //Enable Timer
    TCCR2A |= _BV(WGM20) | _BV(COM2A1); //Set to phase-correct PWM mode, with OC2A (OC2 for m162) as output.
    TCCR2B |= _BV(CS22); //~490Hz PWM output.

    //Ensure SPI is disabled
    SPI_disable();
    
    //Initialise the Serial port:
    Serial_initialise(BAUD_RATE); //SyncScan runs at 9600Baud, use a serial port of your choice as defined in SerialLink.h
      
}




/*
 * EEPROM Validation and Programming Routines
 */

bool checkEEPROMCRC() {
    unsigned char crc = EEPROM_readByte(AstroEQCRC_Address);
    if (crc != calculateEEPROMCRC()) {
        return false; //Invalid CRC
    }
    return true;
}

unsigned char calculateEEPROMCRC(){
    unsigned char crc = 0;
    for (unsigned int addr = AstroEQCRC_Address+1; addr <= EEPROMEnd_Address; addr++) {
        unsigned char data = EEPROM_readByte(addr);
        crc = _crc8_ccitt_update(crc, data);
    }
    if (crc == 0) {
        crc = 1; //Don't allow a CRC value of zero as pre-CRC firmware versions used this value at the CRC address
    }
    return crc;
}

bool checkEEPROM(bool skipCRC){
    char temp[4];
    //First check header:
    EEPROM_readString(temp,3,AstroEQID_Address);
    if(strncmp(temp,"AEQ",3)){
        return false;
    }
    //Then match version number:
    EEPROM_readString(temp,4,AstroEQVer_Address);
    if(strncmp(temp,ASTROEQ_VER_STR,4)){
        return false; //EEPROM needs updating...
    }
    //Then validate CRC (unless skipping)
    if (!skipCRC && !checkEEPROMCRC()) {
        return false;
    }    
    //Then make sure contents makes sense
    if (driverVersion > DRIVER_MAX){
        return false; //invalid value.
    }
    if ((driverVersion == A498x) && microstepConf > 16){
        return false; //invalid value.
    } else if (microstepConf > 32){
        return false; //invalid value.
    }
    if ((cmd.siderealIVal[RA] > MAX_IVAL) || (cmd.siderealIVal[RA] < MIN_IVAL)) {
        return false; //invalid value.
    }
    if ((cmd.siderealIVal[DC] > MAX_IVAL) || (cmd.siderealIVal[DC] < MIN_IVAL)) {
        return false; //invalid value.
    }
    if(cmd.normalGotoSpeed[RA] == 0){
        return false; //invalid value.
    }
    if(cmd.normalGotoSpeed[DC] == 0){
        return false; //invalid value.
    }
    if((cmd.st4SpeedFactor < 1) || (cmd.st4SpeedFactor > 19)){
        return false; //invalid value
    }
    return true;
}

void buildEEPROM(){
    //We initialise with the identifier string
    EEPROM_writeString("AEQ",3,AstroEQID_Address);
    EEPROM_writeString(ASTROEQ_VER_STR,4,AstroEQVer_Address);
    //We don't blank out the EEPROM to allow data recovery when updating firmware to new version.
    //Configuration is now (theoretically) in a valid state, or invalid state with bad crc
}

void storeEEPROM(){
    //Store EEPROM values
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
    EEPROM_writeByte(!disableGearChange, GearEnable_Address);
    EEPROM_writeByte(!allowAdvancedHCDetection, AdvHCEnable_Address);
    EEPROM_writeInt(cmd.st4DecBacklash, DecBacklash_Address);
    EEPROM_writeByte(cmd.st4SpeedFactor, SpeedFactor_Address);
    EEPROM_writeAccelTable(cmd.accelTable[RA],AccelTableLength,AccelTable1_Address);
    EEPROM_writeAccelTable(cmd.accelTable[DC],AccelTableLength,AccelTable2_Address);
    //Then compute and store a valid CRC
    unsigned char crc = calculateEEPROMCRC();
    EEPROM_writeByte(crc,AstroEQCRC_Address);
}









/*
 * Standalone Helpers
 */

byte standaloneModeTest() {
    //We need to test what sort of controller is attached.
    //The IRQ pin on the ST4 connector is used to determine this. It has the following
    //states:
    //   FLOAT      | No hand controller
    //   DRIVE LOW  | Basic hand controller
    //   DRIVE HIGH | Advanced hand controller
    //We can test for each of these states by virtue of having a controllable pull up/down
    //resistor on that pin.
    //If we pull down and the pin stays high, then pin must be driven high (DRIVE HIGH)
    //If we pull up and the pin stays low, then pin must be driven low (DRIVE LOW)
    //Otherwise if pin follows us then it must be floating.

    //To start we check for an advanced controller
    setPinValue(standalonePin[STANDALONE_PULL],LOW); //Pull low
    nop(); // Input synchronizer takes a couple of cycles
    nop();
    nop();
    nop();
    if(allowAdvancedHCDetection && getPinValue(standalonePin[STANDALONE_IRQ])) {
        //Note: Must be an advanced controller as pin stayed high and we are allowing HC detection. (If HC detection is disallowed its because we have no external pull down available)
        return ADVANCED_HC_MODE;
    }
    //Otherwise we check for a basic controller
    setPinValue(standalonePin[STANDALONE_PULL],HIGH); //Convert to external pull-up of IRQ
    nop(); // Input synchronizer takes a couple of cycles
    nop();
    nop();
    nop();
    if(!getPinValue(standalonePin[STANDALONE_IRQ])) {
        //Must be a basic controller as pin stayed low.
        return BASIC_HC_MODE;
    }


    //If we get this far then it is floating, so we assume EQMOD mode
    return EQMOD_MODE;
}


byte checkBasicHCSpeed() {
    //Here we check what the speed is for the basic hand controller.
    //
    //By using both external and internal pull-ups, the following three speeds are possible:
    // +-----------+-----+-----+
    // |  Pull-Up: | Ext | Int |
    // +-----------+-----+-----+
    // | ST-4 Rate |  0  |  0  |
    // |   2x Rate |  1  |  0  |
    // | GoTo Rate |  1  |  1  |
    // +-----------+-----+-----+
    //
    //Note: if we don't have an external pull-up resistor, this function will return either ST-4 Rate (0,0) or GoTo Rate (1,1)
    //
    byte speed;
    if(!getPinValue(standalonePin[STANDALONE_IRQ])) {
        //Must be a ST-4 rate as IRQ pin is low when external pull-up enabled
        speed = CMD_ST4_DEFAULT;
    } else {
        //Otherwise check which high-speed mode it is
        setPinValue(standalonePin[STANDALONE_PULL],LOW);   //Pull external resistor low to drain any line capacitance - mid speed sensing is sensitive!
        for (byte i = 10; i > 0; i--) {
            nop(); //Short delay to drain capacitance.
        }
        setPinDir  (standalonePin[STANDALONE_PULL],INPUT); //Disable external resistor by switching to input
        for (byte i = 200; i > 0; i--) {
            nop(); //Short delay to allow line to rebound.
        }
        if(!getPinValue(standalonePin[STANDALONE_IRQ])) {
            //Must be a 2x rate as IRQ pin goes low when external pull-up disabled
            speed = CMD_ST4_STANDALONE;
        } else {
            speed = CMD_ST4_HIGHSPEED;
        }
    }
    setPinDir  (standalonePin[STANDALONE_PULL],OUTPUT); //Ensure we leave an external pull-up of IRQ.
    setPinValue(standalonePin[STANDALONE_PULL],HIGH);
    //And return the new speed
    return speed;
}



/*
 * AstroEQ firmware main() function
 */

int main(void) {
    //Enable global interrupt flag
    sei();
    //Initialise global variables from the EEPROM
    systemInitialiser();
    
    bool mcuReset = false; //Not resetting the MCU after programming command
    
    bool isST4Move[2] = {false, false};
    char lastST4Pin[2] = {ST4O, ST4O};
    
    unsigned int loopCount = 0;
    char recievedChar = 0; //last character we received
    int8_t decoded = 0; //Whether we have decoded the packet
    char decodedPacket[11]; //temporary store for completed command ready to be processed
    
    for(;;){ //Run loop

        loopCount++; //Counter used to time events based on number of loops.

        if (!standaloneMode && (loopCount == 0) && (progMode == RUNMODE)) { 
            //If we are not in standalone mode and are in run mode, periodically check if we have just entered it
            byte mode = standaloneModeTest();
            if (mode != EQMOD_MODE) {
                //If we have just entered stand-alone mode, then we enable the motors and configure the mount
                motorStop(RA, true); //Ensure both motors are stopped
                motorStop(DC, true);
                
                //This next bit needs to be atomic
                byte oldSREG = SREG; 
                cli();  
                cmd_setjVal(RA, 0x800000); //set the current position to the middle
                cmd_setjVal(DC, 0x800000); //set the current position to the middle
                SREG = oldSREG;
                //End atomic
                //Disable Serial
                Serial_disable();
    
                //We are now in standalone mode.
                standaloneMode = true; 
                
                //Next check what type of hand controller we have
                if (mode == ADVANCED_HC_MODE) {
                    //We pulled low, but pin stayed high
                    //This means we must have an advanced controller actively pulling the line high
                    syntaMode = true; 
                    
                    //Initialise SPI for advanced comms
                    SPI_initialise();
    
                    //And send welcome message
                    char welcome[3];
                    synta_assembleResponse(welcome, '\0', 0 );
                    Serial_writeStr(welcome); //Send error packet to trigger controller state machine.
                    
                } else {
                    //Pin either is being pulled low by us or by something else
                    //This means we might have a basic controller actively pulling the line low
                    //Even if we don't we would default to basic mode.
                    syntaMode = false;
                    
                    //For basic mode we need a pull up resistor on the speed/IRQ line
                    setPinValue(standalonePin[STANDALONE_PULL],HIGH); //Pull high
                    
                    //And then we need to initialise the controller manually so the basic controller can help us move
                    byte state = modeState[defaultSpeedState]; //Extract the default mode - for basic HC we won't change from default mode.
                    setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0   )));
                    setPinDir  (modePins[RA][MODE0],!(state & (byte)(1<<MODE0DIR)));
                    setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0   )));
                    setPinDir  (modePins[DC][MODE0],!(state & (byte)(1<<MODE0DIR)));
                    setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1   )));
                    setPinDir  (modePins[RA][MODE1],!(state & (byte)(1<<MODE1DIR)));
                    setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1   )));
                    setPinDir  (modePins[DC][MODE1],!(state & (byte)(1<<MODE1DIR)));
                    setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2   )));
                    setPinDir  (modePins[RA][MODE2],!(state & (byte)(1<<MODE2DIR)));
                    setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2   )));
                    setPinDir  (modePins[DC][MODE2],!(state & (byte)(1<<MODE2DIR)));
                    
                    
                    Commands_configureST4Speed(CMD_ST4_DEFAULT); //Change the ST4 speeds to default
                    
                    cmd.highSpeedMode[RA] = false;
                    cmd.highSpeedMode[DC] = false;
                    
                    motorEnable(RA); //Ensure the motors are enabled
                    motorEnable(DC);
                    
                    cmd_setGVal      (RA, CMD_GVAL_LOWSPEED_SLEW); //Set both axes to slew mode.
                    cmd_setGVal      (DC, CMD_GVAL_LOWSPEED_SLEW);
                    cmd_setDir       (RA, CMD_FORWARD); //Store the current direction for that axis
                    cmd_updateStepDir(RA ,1);
                    cmd_setDir       (DC, CMD_FORWARD); //Store the current direction for that axis
                    cmd_updateStepDir(RA,1);
                    cmd_setIVal      (RA, cmd.siderealIVal[RA]); //Set RA speed to sidereal
                    
                    readyToGo[RA] = MOTION_START_REQUESTED; //Signal we are ready to go on the RA axis to start sidereal tracking
                    
                    lastST4Pin[RA] = ST4O;
                    lastST4Pin[DC] = ST4O;
                }
            }
            //If we end up in standalone mode, we don't exit until a reset.
        }

        /////////////
        if (syntaMode) {
        //
        // EQMOD or Advanced Hand Controller Synta Mode
        //
            //Check if we need to run the command parser
            
            if ((decoded == -2) || Serial_available()) { //is there a byte in buffer or we still need to process the previous byte?
                //Toggle on the LED to indicate activity.
                togglePin(statusPin);
                #ifdef statusPinShadow_Define
                togglePin(statusPinShdw);
                #endif
                //See what character we need to parse
                if (decoded != -2) {
                    //get the next character in buffer
                    recievedChar = Serial_read();
                } //otherwise we will try to parse the previous character again.
                //Append the current character and try to parse the command
                decoded = synta_recieveCommand(decodedPacket,recievedChar); 
                //Once full command packet received, synta_recieveCommand populates either an error packet (and returns -1), or data packet (returns 1). If incomplete, decodedPacket is unchanged and 0 is returned
                if (decoded != 0){ //Send a response
                    if (decoded > 0){ //Valid Packet, current command is in decoded variable.
                        mcuReset = !decodeCommand(decoded,decodedPacket); //decode the valid packet and populate response.
                    }
                    Serial_writeStr(decodedPacket); //send the response packet (recieveCommand() generated the error packet, or decodeCommand() a valid response)
                } //otherwise command not yet fully received, so wait for next byte
                
                if (mcuReset) {
                    //Special case. We were asked to reset the MCU.
                    Serial_flush(); //Flush out last response.
                    wdt_enable(WDTO_120MS); //WDT has been set to reset MCU.
                    exit(0); //Done
                }
            }
            if (loopCount == 0) {
                setPinValue(statusPin, (progMode == PROGMODE) ? HIGH : LOW);
                #ifdef statusPinShadow_Define
                setPinValue(statusPinShdw, (progMode == PROGMODE) ? HIGH : LOW);
                #endif
            }
            
            //
            //ST4 button handling
            //
            if (!standaloneMode && ((loopCount & 0xFF) == 0)){
                //We only check the ST-4 buttons in EQMOD mode when not doing Go-To, and only every so often - this adds a little bit of debouncing time.
                //Determine which RA ST4 pin if any
                char st4Pin = !getPinValue(st4Pins[RA][ST4N]) ? ST4N : (!getPinValue(st4Pins[RA][ST4P]) ? ST4P : ST4O);
                if (st4Pin != lastST4Pin[RA]) { 
                    //Only update speed/dir if the ST4 pin value has changed.
                    if ((cmd.dir[RA] == CMD_FORWARD) && (readyToGo[RA] == MOTION_START_UPDATABLE)) {
                        //In Synta mode, we only allow the ST-4 port to move forward, and only if EQMOD has configured us previously to be in tracking mode
                        //Update target speed.
                        if (st4Pin != ST4O) {
                            //If RA+/- pressed:
                            cmd_setIVal(RA,cmd.st4RAIVal[(byte)st4Pin]);
                            motorStartRA();
                            isST4Move[RA] = true; //Now doing ST4 movement
                        }
                        else if (isST4Move[RA]) { 
                            //Only return to sidereal speed if we are in an ST4 move.
                            cmd_setIVal(RA,cmd.siderealIVal[RA]);
                            motorStartRA();
                            isST4Move[RA] = false; //No longer ST4 movement
                        }
                    }
                    lastST4Pin[RA] = st4Pin; //Keep track of what the last ST4 pin value was so we can detect a change.
                }
                //Determine which if any DEC ST4 Pin
                st4Pin = !getPinValue(st4Pins[DC][ST4N]) ? ST4N : (!getPinValue(st4Pins[DC][ST4P]) ? ST4P : ST4O);
                if (!cmd.gotoEn[DC] && (st4Pin != lastST4Pin[DC])) {
                    //Only update speed/dir if the ST4 pin value has changed, and we are not in GoTo mode.
                    //Determine the new direction
                    byte dir = CMD_FORWARD;
                    if (st4Pin == ST4N) {
                        //If we need to be going in reverse, switch direction
                        dir = CMD_REVERSE;
                    }
                    if ((cmd.stopped[DC] != CMD_STOPPED) && (cmd.dir[DC] != dir)) {
                        //If we are currently moving in the wrong direction
                        motorStopDC(false); //Stop the Dec motor
                        readyToGo[DC] = MOTION_START_NOTREADY;    //No longer ready to go as we have now deleted any pre-running EQMOD movement.
                        //We don't keep track of last ST4 pin here so that if we were requesting a movement but had to stop
                        //first we can come back in over and over until we have started the movement.
                    } else {
                        //Otherwise we are now free to change to the new required speed.
                        if (st4Pin != ST4O) {
                            //If an ST4 Dec pin is pressed
                            cmd_setIVal(DC,cmd.st4DecIVal);
                            cmd_setDir (DC,dir);
                            cmd_updateStepDir(DC,1);
                            motorStartDC(); //If the motor is currently stopped at this point, this will automatically start them.
                            isST4Move[DC] = true; //Now doing ST4 movement
                        } else if (isST4Move[DC]) {
                            //Otherwise stop th DEC motor
                            motorStopDC(false);
                            isST4Move[DC] = false; //No longer ST4 movement
                        }
                        lastST4Pin[DC] = st4Pin; //Keep track of what the last ST4 pin value was so we can detect a change.
                    }
                }
            }
            
            //Check both axes - loop unraveled for speed efficiency - lots of Flash available.
            if(readyToGo[RA] == MOTION_START_REQUESTED){
                //If we are ready to begin a movement which requires the motors to be reconfigured
                if(cmd.stopped[RA] == CMD_STOPPED){
                    //Once the motor is stopped, we can accelerate to target speed.
                    signed char GVal = cmd.GVal[RA];
                    if (canJumpToHighspeed){
                        //If we are allowed to enable high speed, see if we need to
                        byte state;
                        if (motionIsLowSpeed(GVal)) {
                            //If a low speed mode command
                            state = modeState[SPEEDNORM]; //Select the normal speed mode
                            cmd_updateStepDir(RA,1);
                            cmd.highSpeedMode[RA] = false;
                        } else {
                            state = modeState[SPEEDFAST]; //Select the high speed mode
                            cmd_updateStepDir(RA,cmd.gVal[RA]);
                            cmd.highSpeedMode[RA] = true;
                        }
                        setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0   )));
                        setPinDir  (modePins[RA][MODE0],!(state & (byte)(1<<MODE0DIR)));
                        setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1   )));
                        setPinDir  (modePins[RA][MODE1],!(state & (byte)(1<<MODE1DIR)));
                        setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2   )));
                        setPinDir  (modePins[RA][MODE2],!(state & (byte)(1<<MODE2DIR)));
                    } else {
                        //Otherwise we never need to change the speed
                        cmd_updateStepDir(RA,1); //Just move along at one step per step
                        cmd.highSpeedMode[RA] = false;
                    }
                    if(motionIsSlew(GVal)){
                        //This is the function that enables a slew type move.
                        slewMode(RA); //Slew type
                        readyToGo[RA] = MOTION_START_UPDATABLE;
                    } else {
                        //This is the function for goto mode. You may need to customise it for a different motor driver
                        gotoMode(RA); //Goto Mode
                        readyToGo[RA] = MOTION_START_NOTREADY;
                    }
                } //Otherwise don't start the next movement until we have stopped.
            }
            
            if(readyToGo[DC] == MOTION_START_REQUESTED){
                //If we are ready to begin a movement which requires the motors to be reconfigured
                if(cmd.stopped[DC] == CMD_STOPPED){
                    //Once the motor is stopped, we can accelerate to target speed.
                    signed char GVal = cmd.GVal[DC];
                    if (canJumpToHighspeed){
                        //If we are allowed to enable high speed, see if we need to
                        byte state;
                        if (motionIsLowSpeed(GVal)) {
                            //If a low speed mode command
                            state = modeState[SPEEDNORM]; //Select the normal speed mode
                            cmd_updateStepDir(DC,1);
                            cmd.highSpeedMode[DC] = false;
                        } else {
                            state = modeState[SPEEDFAST]; //Select the high speed mode
                            cmd_updateStepDir(DC,cmd.gVal[DC]);
                            cmd.highSpeedMode[DC] = true;
                        }
                        setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0   )));
                        setPinDir  (modePins[DC][MODE0],!(state & (byte)(1<<MODE0DIR)));
                        setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1   )));
                        setPinDir  (modePins[DC][MODE1],!(state & (byte)(1<<MODE1DIR)));
                        setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2   )));
                        setPinDir  (modePins[DC][MODE2],!(state & (byte)(1<<MODE2DIR)));
                    } else {
                        //Otherwise we never need to change the speed
                        cmd_updateStepDir(DC,1); //Just move along at one step per step
                        cmd.highSpeedMode[DC] = false;
                    }
                    if(motionIsSlew(GVal)){
                        //This is the function that enables a slew type move.
                        slewMode(DC); //Slew type
                        readyToGo[DC] = MOTION_START_UPDATABLE; //We are now in a running mode which speed can be changed without stopping motor (unless a command changes the direction)
                    } else {
                        //This is the function for goto mode.
                        gotoMode(DC); //Goto Mode
                        readyToGo[DC] = MOTION_START_NOTREADY; //We are now in a mode where no further changes can be made to the motor (apart from requesting a stop) until the go-to movement is done.
                    }
                } //Otherwise don't start the next movement until we have stopped.
            }
            
        //////////
        } else {
        //
        // ST4 Basic Hand Controller Mode
        //
            if (loopCount == 0) {
                //we run these checks every so often, not all the time.
                
                //Update status LED
                togglePin(statusPin); //Toggle status pin at roughly constant rate in basic mode as indicator
                #ifdef statusPinShadow_Define
                togglePin(statusPinShdw);
                #endif
                
                //Check the speed
                byte newBasicHCSpeed = checkBasicHCSpeed();
                if (newBasicHCSpeed != cmd.st4Mode) {
                    //Only update speed if changed.
                    Commands_configureST4Speed(newBasicHCSpeed); //Change the ST4 speeds
                    byte state;
                    if (canJumpToHighspeed && (newBasicHCSpeed == CMD_ST4_HIGHSPEED)) {
                        //If we can jump to high torque mode, and we are requesting Go-To s
                        state = modeState[SPEEDFAST]; //Select the high speed mode, then change step modes
                        //RA
                        cmd_updateStepDir(RA,cmd.gVal[RA]);
                        cmd.highSpeedMode[RA] = true;
                        //Dec
                        cmd_updateStepDir(DC,cmd.gVal[DC]);
                        cmd.highSpeedMode[DC] = true;
                    } else {
                        //Otherwise ensure we are in normal speed mode.
                        state = modeState[SPEEDNORM]; //Select the normal speed mode
                        //RA
                        cmd_updateStepDir(RA,1);
                        cmd.highSpeedMode[RA] = false;
                        //Dec
                        cmd_updateStepDir(DC,1);
                        cmd.highSpeedMode[DC] = false;
                    }
                    //RA
                    setPinValue(modePins[RA][MODE0], (state & (byte)(1<<MODE0)));
                    setPinValue(modePins[RA][MODE1], (state & (byte)(1<<MODE1)));
                    setPinValue(modePins[RA][MODE2], (state & (byte)(1<<MODE2)));
                    //Dec
                    setPinValue(modePins[DC][MODE0], (state & (byte)(1<<MODE0)));
                    setPinValue(modePins[DC][MODE1], (state & (byte)(1<<MODE1)));
                    setPinValue(modePins[DC][MODE2], (state & (byte)(1<<MODE2)));
                }
            }
            
            //
            //NESW button handling - uses ST4 pins
            //
            if ((loopCount & 0xFF) == 0){
                //We only check the buttons every so often - this adds a little bit of debouncing time.
                //Determine which if any RA ST4 Pin
                char st4Pin = !getPinValue(st4Pins[RA][ST4N]) ? ST4N : (!getPinValue(st4Pins[RA][ST4P]) ? ST4P : ST4O);
                if ((st4Pin == ST4O) || (st4Pin != lastST4Pin[RA])) { //Only update speed/dir if the ST4 pin value has changed, but also ensure we stop by always doing ST4O!
                    //Determine the new direction
                    byte dir = CMD_FORWARD;
                    if ((st4Pin == ST4N) && (cmd.st4RAReverse == CMD_REVERSE)) {
                        //If we need to be going in reverse, switch direction
                        dir = CMD_REVERSE;
                    }
                    byte oldSREG = SREG;
                    cli(); //We are reading motor ISR values, so ensure we are atomic.
                    unsigned int currentSpeed = currentMotorSpeed(RA);
                    SREG = oldSREG; //End atomic
                    if ((cmd.stopped[RA] != CMD_STOPPED) && (cmd.dir[RA] != dir) && (currentSpeed < cmd.minSpeed[RA])) {
                        //If we are currently moving in the wrong direction and are traveling too fast to instantly reverse
                        motorStopRA(false);
                        //We don't keep track of last ST4 pin here so that if we were requesting a movement but had to stop
                        //first we can come back in over and over until we have started the movement.
                    } else {
                        //Otherwise we are now free to change to the new required speed
                        // - If no RA button is pressed, go at sidereal rate
                        // - Otherwise go at rate corresponding with the pressed button
                        cmd_setIVal(RA, (st4Pin == ST4O) ? cmd.siderealIVal[RA] : cmd.st4RAIVal[(byte)st4Pin]);
                        cmd_setDir(RA,dir);
                        cmd_updateStepDir(RA,cmd.highSpeedMode[RA] ? cmd.gVal[RA] : 1);
                        if ((st4Pin == ST4O) && (cmd.st4Mode == CMD_ST4_HIGHSPEED)) {
                            motorStopRA(false); //If no buttons pressed and in high speed mode, we stop entirely rather than going to tracking
                                                //This ensures that the motors stop if the hand controller is subsequently unplugged.
                        } else {
                            motorStartRA(); //If the motor is currently stopped at this point, this will automatically start them.
                        }
                        lastST4Pin[RA] = st4Pin; //Keep track of what the last ST4 pin value was so we can detect a change.
                    }
                }
                //Determine which if any DEC ST4 Pin
                st4Pin = !getPinValue(st4Pins[DC][ST4N]) ? ST4N : (!getPinValue(st4Pins[DC][ST4P]) ? ST4P : ST4O);
                if ((st4Pin == ST4O) || (st4Pin != lastST4Pin[DC])) { //Only update speed/dir if the ST4 pin value has changed, but also ensure we stop by always doing ST4O!
                    //Determine the new direction
                    byte dir = CMD_FORWARD;
                    if (st4Pin == ST4N) {
                        //If we need to be going in reverse, switch direction
                        dir = CMD_REVERSE;
                    }
                    byte oldSREG = SREG;
                    cli(); //We are reading motor ISR values, so ensure we are atomic.
                    unsigned int currentSpeed = currentMotorSpeed(DC);
                    SREG = oldSREG; //End atomic
                    if ((cmd.stopped[DC] != CMD_STOPPED) && (cmd.dir[DC] != dir) && (currentSpeed < cmd.minSpeed[DC])) {
                        //If we are currently moving in the wrong direction and are traveling too fast to instantly reverse
                        motorStopDC(false);
                        //We don't keep track of last ST4 pin here so that if we were requesting a movement but had to stop
                        //first we can come back in over and over until we have started the movement.
                    } else {
                        //Otherwise we are now free to change to the new required speed.
                        if (st4Pin != ST4O) {
                            //If an ST4 Dec pin is pressed
                            cmd_setIVal(DC,cmd.st4DecIVal);
                            cmd_setDir (DC,dir);
                            cmd_updateStepDir(DC,cmd.highSpeedMode[DC] ? cmd.gVal[DC] : 1);
                            motorStartDC(); //If the motor is currently stopped at this point, this will automatically start them.
                        } else {
                            //Otherwise stop th DEC motor
                            motorStopDC(false);
                        }
                        lastST4Pin[DC] = st4Pin; //Keep track of what the last ST4 pin value was so we can detect a change.
                    }
                }
            }
        ///////////
        }
        
        
    }//End of run loop
}


/*
 * Decode and Perform the Command
 */

bool decodeCommand(char command, char* buffer){ //each command is axis specific. The axis being modified can be retrieved by calling synta_axis()
    unsigned long responseData = 0; //data for response
    bool success = true;
    byte axis = synta_getaxis();
    unsigned int correction;
    byte oldSREG;
    if ((progMode == RUNMODE) && (command != 'O')) {
        //If any command other than programming entry request is sent, reset the entry count.
        progModeEntryCount = 0;
    }
    switch(command) {
        case 'e': //read-only, return the eVal (version number)
            responseData = cmd.eVal[axis]; //response to the e command is stored in the eVal function for that axis.
            break;
        case 'a': //read-only, return the aVal (steps per axis)
            responseData = cmd.aVal[axis]; //response to the a command is stored in the aVal function for that axis.
            break;
        case 'b': //read-only, return the bVal (sidereal step rate)
            responseData = cmd.bVal[axis]; //response to the b command is stored in the bVal function for that axis.
            if (progMode == RUNMODE) {
                //If not in programming mode, we need to apply a correction factor to ensure that calculations in EQMOD round correctly
                correction = (cmd.siderealIVal[axis] << 1);
                responseData = (responseData * (correction+1))/correction; //account for rounding inside Skywatcher DLL.
            }
            break;
        case 'g': //read-only, return the gVal (high speed multiplier)
            responseData = cmd.gVal[axis]; //response to the g command is stored in the gVal function for that axis.
            break;
        case 's': //read-only, return the sVal (steps per worm rotation)
            responseData = cmd.sVal[axis]; //response to the s command is stored in the sVal function for that axis.
            break;
        case 'f': //read-only, return the fVal (axis status)
            responseData = cmd_fVal(axis); //response to the f command is stored in the fVal function for that axis.
            break;
        case 'j': //read-only, return the jVal (current position)
            oldSREG = SREG; 
            cli();  //The next bit needs to be atomic, just in case the motors are running
            responseData = cmd.jVal[axis]; //response to the j command is stored in the jVal function for that axis.
            SREG = oldSREG;
            break;
        case 'K': //stop the motor, return empty response
            motorStop(axis,0); //normal ISR based deceleration trigger.
            readyToGo[axis] = MOTION_START_NOTREADY;
            break;
        case 'L':
            motorStop(axis,1); //emergency axis stop.
            motorDisable(axis); //shutdown driver power.
            break;
        case 'G': //set mode and direction, return empty response
            {
                signed char GVal = (buffer[0] - '0');
                cmd_setGVal(axis, GVal); //Store the current mode for the axis
                cmd_setDir(axis, (buffer[1] != '0') ? CMD_REVERSE : CMD_FORWARD); //Store the current direction for that axis
                readyToGo[axis] = MOTION_START_NOTREADY;
                //We may need to update flags early...
                if(cmd.stopped[axis] == CMD_STOPPED){
                    //If the motor is stopped, we update the high-speed mode flag immediately. This is to help IndiEQMOD in certain edge cases where it issues :G then :f
                    if (canJumpToHighspeed) {
                        //If we are allowed to enable high speed, see if we need to
                        if (motionIsLowSpeed(GVal)) {
                            //If a low speed mode command
                            cmd.highSpeedMode[axis] = false; //Change flag to normal speed mode (we aren't actually in this mode yet, we will change on next :J command)
                        } else {
                            cmd.highSpeedMode[axis] = true; //Change flag to high speed mode (we aren't actually in this mode yet, we will change on next :J command)
                        }
                    }
                }
            }
            break;
        case 'H': //set goto position, return empty response (this sets the number of steps to move from current position if in goto mode)
            cmd_setHVal(axis, synta_hexToLong(buffer)); //set the goto position container (convert string to long first)
            readyToGo[axis] = MOTION_START_NOTREADY;
            break;
        case 'I': //set slew speed, return empty response (this sets the speed to move at if in slew mode)
            responseData = synta_hexToLong(buffer); //convert string to long first
            if (responseData < cmd.accelTable[axis][AccelTableLength-1].speed) {
                //Limit the IVal to the largest speed in the acceleration table to prevent sudden rapid acceleration at the end.
                responseData = cmd.accelTable[axis][AccelTableLength-1].speed; 
            }
            cmd_setIVal(axis, responseData); //set the speed container
            responseData = 0;
            if (readyToGo[axis] == MOTION_START_UPDATABLE) {
                //If we are in a running mode which allows speed update without motor reconfiguration
                motorStart(axis); //Simply update the speed.
            } else {
                //Otherwise we are no longer ready to go until the next :J command is received
                readyToGo[axis] = MOTION_START_NOTREADY;
            }
            break;
        case 'E': //set the current position, return empty response
            oldSREG = SREG; 
            cli();  //The next bit needs to be atomic, just in case the motors are running
            cmd_setjVal(axis, synta_hexToLong(buffer)); //set the current position (used to sync to what EQMOD thinks is the current position at startup
            SREG = oldSREG;
            break;
        case 'F': //Enable the motor driver, return empty response
            if (progMode == RUNMODE) { //only allow motors to be enabled outside of programming mode.
                motorEnable(axis); //This enables the motors - gives the motor driver board power
            } else {
                command = '\0'; //force sending of error packet!.
            }
            break;
        case 'V': //Set the polarscope LED brightness
            polarscopeDutyRegister(synta_hexToByte(buffer));
            break;
            
        //Command required for entering programming mode. All other programming commands cannot be used when progMode = 0 (normal ops)
        case 'O': //Control GPIO1 or Programming Mode
            if (axis == RA) {
                //:O commands to the DC axis control GPIO1 (SNAP2 port)
                setPinValue(snapPin,(buffer[0] - '0'));
                progModeEntryCount = 0;
            } else {
                //Only :O commands to the RA axis are accepted. DC enters and controls programming mode on special sequence.
                progMode = buffer[0] - '0';              //MODES:  0 = Normal Ops (EQMOD). 1 = Validate EEPROM. 2 = Store to EEPROM. 3 = Rebuild EEPROM
                if (progModeEntryCount < 19) {
                    //If we haven't sent enough entry commands to switch into programming mode
                    if ((progMode != PROGMODE) && (progMode != STOREMODE) && (progMode != REBUILDMODE)) {
                        //If we sent an entry command that asks for normal operation, reset the entry count.
                        progModeEntryCount = 0;
                    } else {
                        //Otherwise increment the count of entry requests.
                        progModeEntryCount = progModeEntryCount + 1;
                    }
                    command = '\0'; //force sending of error packet when not in programming mode (so that EQMOD knows not to use SNAP1 interface).
                } else {
                    progModeEntryCount = 20;
                    if (progMode != RUNMODE) {
                        motorStop(RA,1); //emergency axis stop.
                        motorDisable(RA); //shutdown driver power.
                        motorStop(DC,1); //emergency axis stop.
                        motorDisable(DC); //shutdown driver power.
                        readyToGo[RA] = MOTION_START_NOTREADY;
                        readyToGo[DC] = MOTION_START_NOTREADY;
                    } else { //reset the uC to return to normal ops mode.
                        success = false;
                    }
                }
            }
            break;

        default:
            //Prevent any chance of accidentally running configuration commands when not in programming mode.
            if (progMode != RUNMODE) {
                //The following are used for configuration ----------
                switch(command) {
                    case 'A': //store the aVal (steps per axis)
                        cmd_setaVal(axis, synta_hexToLong(buffer)); //store aVal for that axis.
                        break;
                    case 'B': //store the bVal (sidereal rate)
                        cmd_setbVal(axis, synta_hexToLong(buffer)); //store bVal for that axis.
                        break;
                    case 'S': //store the sVal (steps per worm rotation)
                        cmd_setsVal(axis, synta_hexToLong(buffer)); //store sVal for that axis.
                        break;
                    case 'n': //return the IVal (EQMOD Speed at sidereal)
                        responseData = cmd.siderealIVal[axis];
                        break;
                    case 'N': //store the IVal (EQMOD Speed at sidereal)
                        cmd_setsideIVal(axis, synta_hexToLong(buffer)); //store sVal for that axis.
                        break;
                    case 'd': //return the driver version or step mode
                        if (axis) {
                            responseData = microstepConf; 
                        } else {
                            responseData = driverVersion;
                        }
                        break;
                    case 'D': //store the driver version and step modes
                        if (axis) {
                            microstepConf = synta_hexToByte(buffer); //store step mode.
                            canJumpToHighspeed = (microstepConf >= 8) && !disableGearChange; //Gear change is enabled if the microstep mode can change by a factor of 8.
                        } else {
                            driverVersion = synta_hexToByte(buffer); //store driver version.
                        }
                        break;
                    case 'r': //return the DEC backlash or st4 speed factor
                        if (axis) {
                            responseData = cmd.st4DecBacklash; 
                        } else {
                            responseData = cmd.st4SpeedFactor;
                        }
                        break;
                    case 'R': //store the DEC backlash or st4 speed factor
                        if (axis) {
                            unsigned long dataIn = synta_hexToLong(buffer); //store step mode.
                            if (dataIn > 65535) {
                                command = '\0'; //If the step rate is out of range, force an error response packet.
                            } else {
                                cmd_setst4DecBacklash(dataIn); //store st4 speed factor
                            }
                        } else {
                            byte factor = synta_hexToByte(buffer);
                            if ((factor > 19) || (factor < 1)) {
                                command = '\0'; //If the factor is out of range, force an error response packet.
                            } else {
                                cmd_setst4SpeedFactor(factor); //store st4 speed factor
                            }
                        }
                        break;
                    case 'z': //return the Goto speed
                        responseData = cmd.normalGotoSpeed[axis];
                        break;
                    case 'Z': //return the Goto speed factor
                        cmd.normalGotoSpeed[axis] = synta_hexToByte(buffer); //store the goto speed factor
                        break;
                    case 'c': //return the axisDirectionReverse
                        responseData = encodeDirection[axis];
                        break;
                    case 'C': //store the axisDirectionReverse
                        encodeDirection[axis] = buffer[0] - '0'; //store sVal for that axis.
                        break;
                    case 'q': //return the disableGearChange/allowAdvancedHCDetection setting  
                        if (axis) {
                            responseData = disableGearChange; 
                            canJumpToHighspeed = (microstepConf >= 8) && !disableGearChange; //Gear change is enabled if the microstep mode can change by a factor of 8.
                        } else {
                            responseData = allowAdvancedHCDetection;
                        }
                        break;
                    case 'Q': //store the disableGearChange/allowAdvancedHCDetection setting
                        if (axis) {
                            disableGearChange = synta_hexToByte(buffer); //store whether we can change gear
                        } else {
                            allowAdvancedHCDetection = synta_hexToByte(buffer); //store whether to allow advanced hand controller detection
                        }
                        break;
                    case 'x': {  //return the accelTable
                        Inter responsePack = {0};
                        responsePack.lowByter.integer = cmd.accelTable[axis][accelTableIndex[axis]].speed;
                        responsePack.highByter.low = cmd.accelTable[axis][accelTableIndex[axis]].repeats; 
                        responseData = responsePack.integer;
                        accelTableIndex[axis]++; //increment the index so we don't have to send :Y commands for every address if reading sequentially.
                        if (accelTableIndex[axis] >= AccelTableLength) {
                            accelTableIndex[axis] = 0; //Wrap around
                        }
                        break;
                    }
                    case 'X': { //store the accelTable value for address set by 'Y', or next address after last 'X'
                        unsigned long dataIn = synta_hexToLong(buffer);
                        cmd.accelTable[axis][accelTableIndex[axis]].speed = (unsigned int)dataIn; //lower two bytes is speed
                        cmd.accelTable[axis][accelTableIndex[axis]].repeats = (byte)(dataIn>>16); //upper byte is repeats.
                        accelTableIndex[axis]++; //increment the index so we don't have to send :Y commands for every address if programming sequentially.
                        if (accelTableIndex[axis] >= AccelTableLength) {
                            accelTableIndex[axis] = 0; //Wrap around
                        }
                        break;
                    }
                    case 'Y': //store the accelTableIndex value
                        //Use axis=0 to set which address we are accessing (we'll re purpose accelTableIndex[RA] in prog mode for this)
                        accelTableIndex[axis] = synta_hexToByte(buffer);
                        if (accelTableIndex[axis] >= AccelTableLength) {
                            command = '\0'; //If the address out of range, force an error response packet.
                        }
                        break;
                    case 'T': //set mode, return empty response
                        if (progMode == STOREMODE) {
                            //proceed with EEPROM write
                            storeEEPROM();
                        } else if (progMode == REBUILDMODE) {
                            //repair EEPROM
                            buildEEPROM();
                        } else if (progMode == PROGMODE) {
                            //check if EEPROM contains valid data. (Skip EEPROM CRC checks if requested)
                            if (!checkEEPROM(buffer[0]-'0')) { 
                                //If invalid EEPROM configuration (excluding CRC if skipping CRC)
                                command = '\0'; //force sending of an error packet if invalid EEPROM
                            } else if (!checkEEPROMCRC()) {
                                //If invalid EEPROM configuration is now only due to CRC
                                EEPROM_writeByte(calculateEEPROMCRC(),AstroEQCRC_Address); //Recalculate and store the correct CRC
                                //We don't report an error as the CRC is now ready.
                            }
                        }
                        break;
                    //---------------------------------------------------
                    default: //Return empty response (deals with commands that don't do anything before the response sent (i.e 'J', 'R'), or do nothing at all (e.g. 'M') )
                        break;
                }
            }
            break;
    }
  
    synta_assembleResponse(buffer, command, responseData); //generate correct response (this is required as is)
    
    if ((command == 'J') && (progMode == RUNMODE)) { //J tells us we are ready to begin the requested movement.
        readyToGo[axis] = MOTION_START_REQUESTED; //So signal we are ready to go and when the last movement completes this one will execute.
        if (motionIsGoto(cmd.GVal[axis])){
            //If go-to mode requested
            cmd_setGotoEn(axis,CMD_ENABLED);
        }
    }
    return success;
}










void motorEnable(byte axis){
    motorStop(axis,true); //First perform an "emergency" stop which basically disengages the motors and clears all running flags.
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
    
    if (cmd.highSpeedMode[axis]) {
        //Additionally in order to maintain the same speed profile in high-speed mode, we actually increase the profile repeats by a factor of sqrt(8)
        //compared with running in normal-speed mode. See Atmel AVR466 app note for calculation.
        decelerationLength = decelerationLength * 3; //multiply by 3 as it is approx sqrt(8)
    }
    
    byte dirMagnitude = abs(cmd.stepDir[axis]);
    byte dir = cmd.dir[axis];

    if (cmd.HVal[axis] < 2*dirMagnitude){
        cmd_setHVal(axis,2*dirMagnitude);
    }

    decelerationLength = decelerationLength * dirMagnitude;
    //deceleration length is here a multiple of stepDir.
    unsigned long HVal = cmd.HVal[axis];
    unsigned long halfHVal = (HVal >> 1);
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
    gotoPosn[axis] = cmd.jVal[axis] + ((dir == CMD_REVERSE) ? -HVal : HVal); //current position + relative change - deceleration region
    
    cmd_setIVal(axis, gotoSpeed);
    clearGotoDecelerating(axis);
    setGotoRunning(axis); //start the goto.
    motorStart(axis); //Begin PWM
}

inline void timerEnable(byte motor) {
    if (motor == RA) {
        timerPrescalarRegister(RA, timerPrescalarRegister(RA) & ~((1<<CSn2) | (1<<CSn1)) );//00x
        timerPrescalarRegister(RA, timerPrescalarRegister(RA) |  (            (1<<CSn0)) );//xx1
    } else {
        timerPrescalarRegister(DC, timerPrescalarRegister(DC) & ~((1<<CSn2) | (1<<CSn1)) );//00x
        timerPrescalarRegister(DC, timerPrescalarRegister(DC) |  (            (1<<CSn0)) );//xx1
    }
}

inline void timerDisable(byte motor) {
    if (motor == RA) {
        interruptControlRegister(RA, interruptControlRegister(RA) & ~interruptControlBitMask(RA)); //Disable timer interrupt
        timerPrescalarRegister(RA, timerPrescalarRegister(RA) & ~((1<<CSn2) | (1<<CSn1) | (1<<CSn0)));//000
    } else {
        interruptControlRegister(DC, interruptControlRegister(DC) & ~interruptControlBitMask(DC)); //Disable timer interrupt
        timerPrescalarRegister(DC, timerPrescalarRegister(DC) & ~((1<<CSn2) | (1<<CSn1) | (1<<CSn0)));//000
    }
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
    
    interruptControlRegister(RA, interruptControlRegister(RA) & ~interruptControlBitMask(RA)); //Disable timer interrupt
    currentIVal = currentMotorSpeed(RA);
    interruptControlRegister(RA, interruptControlRegister(RA) | interruptControlBitMask(RA)); //enable timer interrupt
    
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
    
    interruptControlRegister(RA, interruptControlRegister(RA) & ~interruptControlBitMask(RA)); //Disable timer interrupt
    cmd.currentIVal[RA] = cmd.IVal[RA];
    currentMotorSpeed(RA, startSpeed);
    cmd.stopSpeed[RA] = stoppingSpeed;
    setPinValue(dirPin[RA],(encodeDirection[RA] != cmd.dir[RA]));
    
    if(cmd.stopped[RA]) { //if stopped, configure timers
        irqToNextStep(RA, 1);
        accelTableRepeatsLeft[RA] = cmd.accelTable[RA][0].repeats; //If we are stopped, we must do the required number of repeats for the first entry in the speed table.
        accelTableIndex[RA] = 0;
        distributionSegment(RA, 0);
        timerCountRegister(RA, 0);
        interruptOVFCount(RA, timerOVF[RA][0]);
        timerEnable(RA);
        cmd_setStopped(RA, CMD_RUNNING);
    }
    interruptControlRegister(RA, interruptControlRegister(RA) | interruptControlBitMask(RA)); //enable timer interrupt
}

void motorStartDC(){
    unsigned int IVal = cmd.IVal[DC];
    unsigned int currentIVal;
    interruptControlRegister(DC, interruptControlRegister(DC) & ~interruptControlBitMask(DC)); //Disable timer interrupt
    currentIVal = currentMotorSpeed(DC);
    interruptControlRegister(DC, interruptControlRegister(DC) | interruptControlBitMask(DC)); //enable timer interrupt
    
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
    
    interruptControlRegister(DC, interruptControlRegister(DC) & ~interruptControlBitMask(DC)); //Disable timer interrupt
    cmd.currentIVal[DC] = cmd.IVal[DC];
    currentMotorSpeed(DC, startSpeed);
    cmd.stopSpeed[DC] = stoppingSpeed;
    setPinValue(dirPin[DC],(encodeDirection[DC] != cmd.dir[DC]));
    
    if(cmd.stopped[DC]) { //if stopped, configure timers
        irqToNextStep(DC, 1);
        accelTableRepeatsLeft[DC] = cmd.accelTable[DC][0].repeats; //If we are stopped, we must do the required number of repeats for the first entry in the speed table.
        accelTableIndex[DC] = 0;
        distributionSegment(DC, 0);
        timerCountRegister(DC, 0);
        interruptOVFCount(DC, timerOVF[DC][0]);
        timerEnable(DC);
        cmd_setStopped(DC, CMD_RUNNING);
    }
    interruptControlRegister(DC, interruptControlRegister(DC) | interruptControlBitMask(DC)); //enable timer interrupt
}

//As there is plenty of FLASH left, then to improve speed, I have created two motorStop functions (one for RA and one for DEC)
void motorStop(byte motor, byte emergency){
    if (motor == RA) {
        motorStopRA(emergency);
    } else {
        motorStopDC(emergency);
    }
}

void motorStopRA(bool emergency){
    if (emergency) {
        //trigger instant shutdown of the motor in an emergency.
        timerDisable(RA);
        cmd_setGotoEn(RA,CMD_DISABLED); //Not in goto mode.
        cmd_setStopped(RA,CMD_STOPPED); //mark as stopped
        cmd_setGVal(RA,CMD_GVAL_LOWSPEED_SLEW); //Switch back to slew mode
        readyToGo[RA] = MOTION_START_NOTREADY;
        clearGotoRunning(RA);
    } else if (!cmd.stopped[RA]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
        //trigger ISR based deceleration
        //readyToGo[RA] = MOTION_START_NOTREADY;
        byte oldSREG = SREG;
        cli();
        cmd_setGotoEn(RA,CMD_DISABLED); //No longer in goto mode.
        clearGotoRunning(RA);
        if (motionIsGoto(cmd.GVal[RA])){
            cmd_setGVal(RA,CMD_GVAL_LOWSPEED_SLEW); //Switch back to slew mode (if we just finished a GoTo)
        }
        //interruptControlRegister(RA) &= ~interruptControlBitMask(RA); //Disable timer interrupt
        if(cmd.currentIVal[RA] < cmd.minSpeed[RA]){
            if(cmd.stopSpeed[RA] > cmd.minSpeed[RA]){
                cmd.stopSpeed[RA] = cmd.minSpeed[RA];
            }
        }/* else {
            stopSpeed[RA] = cmd.currentIVal[RA];
        }*/
        cmd.currentIVal[RA] = cmd.stopSpeed[RA] + 1;//cmd.stepIncrement[motor];
        SREG = oldSREG;
        //interruptControlRegister(RA) |= interruptControlBitMask(RA); //enable timer interrupt
    }
}

void motorStopDC(bool emergency){
    if (emergency) {
        //trigger instant shutdown of the motor in an emergency.
        timerDisable(DC);
        cmd_setGotoEn(DC,CMD_DISABLED); //Not in goto mode.
        cmd_setStopped(DC,CMD_STOPPED); //mark as stopped
        cmd_setGVal(DC,CMD_GVAL_LOWSPEED_SLEW); //Switch back to slew mode
        readyToGo[DC] = MOTION_START_NOTREADY;
        clearGotoRunning(DC);
    } else if (!cmd.stopped[DC]){  //Only stop if not already stopped - for some reason EQMOD stops both axis when slewing, even if one isn't currently moving?
        //trigger ISR based deceleration
        //readyToGo[DC] = MOTION_START_NOTREADY;
        byte oldSREG = SREG;
        cli();
        cmd_setGotoEn(DC,CMD_DISABLED); //No longer in goto mode.
        clearGotoRunning(DC);
        if (motionIsGoto(cmd.GVal[DC])){
	        cmd_setGVal(DC,CMD_GVAL_LOWSPEED_SLEW); //Switch back to slew mode (if we just finished a GoTo)
        }
        //interruptControlRegister(DC) &= ~interruptControlBitMask(DC); //Disable timer interrupt
        if(cmd.currentIVal[DC] < cmd.minSpeed[DC]){
            if(cmd.stopSpeed[DC] > cmd.minSpeed[DC]){
                cmd.stopSpeed[DC] = cmd.minSpeed[DC];
            }
        }/* else {
        stopSpeed[DC] = cmd.currentIVal[DC];
        }*/
        cmd.currentIVal[DC] = cmd.stopSpeed[DC] + 1;//cmd.stepIncrement[motor];
        SREG = oldSREG;
        //interruptControlRegister(DC) |= interruptControlBitMask(DC); //enable timer interrupt
    }
}

//Timer Interrupt-----------------------------------------------------------------------------
void configureTimer(){
    interruptControlRegister(DC, 0); //disable all timer interrupts.
#if defined(__AVR_ATmega162__)
    interruptControlRegister(RA, interruptControlRegister(RA) & 0b00000011); //for 162, the lower 2 bits of the RA register control another timer, so leave them alone.
#else
    interruptControlRegister(RA, 0);
#endif
    //set to ctc mode (0100)
    TCCR1A = 0;//~((1<<WGM11) | (1<<WGM10));
    TCCR1B = ((1<<WGM12) | (1<<WGM13));
    TCCR3A = 0;//~((1<<WGM31) | (1<<WGM30));
    TCCR3B = ((1<<WGM32) | (1<<WGM33));
}



/*Timer Interrupt Vector*/
ISR(TIMER3_CAPT_vect) {
    
    //Load the number of interrupts until the next step
    unsigned int irqToNext = irqToNextStep(DC)-1;
    //Check if we are ready to step
    if (irqToNext == 0) {
        //Once the required number of interrupts have occurred...
        
        //First update the interrupt base rate using our distribution array. 
        //This affords a more accurate sidereal rate by dithering the interrupt rate to get higher resolution.
        byte timeSegment = distributionSegment(DC); //Get the current time segment
        
        /* 
        byte index = ((DecimalDistnWidth-1) & timeSegment) >> 1; //Convert time segment to array index
        interruptOVFCount(DC) = timerOVF[DC][index]; //Update interrupt base rate.
        */// Below is optimised version of above:
        byte index = ((DecimalDistnWidth-1) << 1) & timeSegment; //Convert time segment to array index
        interruptOVFCount(DC, *(int*)((byte*)timerOVF[DC] + index)); //Update interrupt base rate.
        
        distributionSegment(DC, timeSegment + 1); //Increment time segment for next time.

        unsigned int currentSpeed = currentMotorSpeed(DC); //Get the current motor speed
        irqToNextStep(DC, currentSpeed); //Update interrupts to next step to be the current speed in case it changed (accel/decel)
        
        if (getPinValue(stepPin[DC])){
            //If the step pin is currently high...
            
            setPinValue(stepPin[DC],LOW); //set step pin low to complete step
            
            //Then increment our encoder value by the required amount of encoder values per step (1 for low speed, 8 for high speed)
            //and in the correct direction (+ = forward, - = reverse).
            unsigned long jVal = cmd.jVal[DC]; 
            jVal = jVal + cmd.stepDir[DC];
            cmd.jVal[DC] = jVal;
            
            if(gotoRunning(DC) && !gotoDecelerating(DC)){
                //If we are currently performing a Go-To and haven't yet started deceleration...
                if (gotoPosn[DC] == jVal){ 
                    //If we have reached the start deceleration marker...
                    setGotoDecelerating(DC); //Mark that we have started deceleration.
                    cmd.currentIVal[DC] = cmd.stopSpeed[DC]+1; //Set the new target speed to slower than the stop speed to cause deceleration to a stop.
                    accelTableRepeatsLeft[DC] = 0;
                }
            } 
            
            if (currentSpeed > cmd.stopSpeed[DC]) {
                //If the current speed is now slower than the stopping speed, we can stop moving. So...
                if(gotoRunning(DC)){ 
                    //if we are currently running a goto... 
                    cmd_setGotoEn(DC,CMD_DISABLED); //Switch back to slew mode 
                    clearGotoRunning(DC); //And mark goto status as complete
                } //otherwise don't as it cancels a 'goto ready' state 
                
                cmd_setStopped(DC,CMD_STOPPED); //mark as stopped 
                timerDisable(DC);  //And stop the interrupt timer.
            } 
        } else {
            //If the step pin is currently low...
            setPinValue(stepPin[DC],HIGH); //Set it high to start next step.
            
            //If the current speed is not the target speed, then we are in the accel/decel phase. So...
            byte repeatsReqd = accelTableRepeatsLeft[DC]; //load the number of repeats left for this accel table entry
            if (repeatsReqd == 0) { 
                //If we have done enough repeats for this entry
                unsigned int targetSpeed = cmd.currentIVal[DC]; //Get the target speed
                if (currentSpeed > targetSpeed) {
                    //If we are going too slow
                    byte accelIndex = accelTableIndex[DC]; //Load the acceleration table index
                    if (accelIndex >= AccelTableLength-1) {
                        //If we are at the top of the accel table
                        currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                        accelIndex = AccelTableLength-1; //Ensure index remains in bounds.
                    } else {
                        //Otherwise, we need to accelerate.
                        accelIndex = accelIndex + 1; //Move to the next index
                        accelTableIndex[DC] = accelIndex; //Save the new index back
                        currentSpeed = cmd.accelTable[DC][accelIndex].speed;  //load the new speed from the table
                        if (currentSpeed <= targetSpeed) {
                            //If the new value is too fast
                            currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                        } else {
                            //Load the new number of repeats required
                            if (cmd.highSpeedMode[DC]) {
                                //When in high-speed mode, we need to multiply by sqrt(8) ~= 3 to compensate for the change in steps per rev of the motor
                                accelTableRepeatsLeft[DC] = cmd.accelTable[DC][accelIndex].repeats * 3 + 2;
                            } else {
                                accelTableRepeatsLeft[DC] = cmd.accelTable[DC][accelIndex].repeats;
                            }
                        }
                    }
                } else if (currentSpeed < targetSpeed) {
                    //If we are going too fast
                    byte accelIndex = accelTableIndex[DC]; //Load the acceleration table index
                    if (accelIndex == 0) {
                        //If we are at the bottom of the accel table
                        currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                    } else {
                        //Otherwise, we need to decelerate.
                        accelIndex = accelIndex - 1; //Move to the next index
                        accelTableIndex[DC] = accelIndex; //Save the new index back
                        currentSpeed = cmd.accelTable[DC][accelIndex].speed;  //load the new speed from the table
                        if (currentSpeed >= targetSpeed) {
                            //If the new value is too slow
                            currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                        } else {
                            //Load the new number of repeats required
                            if (cmd.highSpeedMode[DC]) {
                                //When in high-speed mode, we need to multiply by sqrt(8) ~= 3 to compensate for the change in steps per rev of the motor
                                accelTableRepeatsLeft[DC] = cmd.accelTable[DC][accelIndex].repeats * 3 + 2;
                            } else {
                                accelTableRepeatsLeft[DC] = cmd.accelTable[DC][accelIndex].repeats;
                            }
                        }
                    }
                }
                currentMotorSpeed(DC, currentSpeed); //Update the current speed in case it has changed.
            } else {
                //Otherwise one more repeat done.
                accelTableRepeatsLeft[DC] = repeatsReqd - 1;
            }
        }
    } else {
        //The required number of interrupts have not yet occurred...
        irqToNextStep(DC, irqToNext); //Update the number of IRQs remaining until the next step.
    }   


}






/*Timer Interrupt Vector*/
ISR(TIMER1_CAPT_vect) {
    
    //Load the number of interrupts until the next step
    unsigned int irqToNext = irqToNextStep(RA)-1;
    //Check if we are ready to step
    if (irqToNext == 0) {
        //Once the required number of interrupts have occurred...
        
        //First update the interrupt base rate using our distribution array. 
        //This affords a more accurate sidereal rate by dithering the interrupt rate to get higher resolution.
        byte timeSegment = distributionSegment(RA); //Get the current time segment
        
        /* 
        byte index = ((DecimalDistnWidth-1) & timeSegment) >> 1; //Convert time segment to array index
        interruptOVFCount(RA) = timerOVF[RA][index]; //Update interrupt base rate.
        */// Below is optimised version of above:
        byte index = ((DecimalDistnWidth-1) << 1) & timeSegment; //Convert time segment to array index
        interruptOVFCount(RA, *(int*)((byte*)timerOVF[RA] + index)); //Update interrupt base rate.
        
        distributionSegment(RA, timeSegment + 1); //Increment time segment for next time.

        unsigned int currentSpeed = currentMotorSpeed(RA); //Get the current motor speed
        irqToNextStep(RA, currentSpeed); //Update interrupts to next step to be the current speed in case it changed (accel/decel)
        
        if (getPinValue(stepPin[RA])){
            //If the step pin is currently high...
            
            setPinValue(stepPin[RA],LOW); //set step pin low to complete step
            
            //Then increment our encoder value by the required amount of encoder values per step (1 for low speed, 8 for high speed)
            //and in the correct direction (+ = forward, - = reverse).
            unsigned long jVal = cmd.jVal[RA]; 
            jVal = jVal + cmd.stepDir[RA];
            cmd.jVal[RA] = jVal;
            
            if(gotoRunning(RA) && !gotoDecelerating(RA)){
                //If we are currently performing a Go-To and haven't yet started deceleration...
                if (gotoPosn[RA] == jVal){ 
                    //If we have reached the start deceleration marker...
                    setGotoDecelerating(RA); //Mark that we have started deceleration.
                    cmd.currentIVal[RA] = cmd.stopSpeed[RA]+1; //Set the new target speed to slower than the stop speed to cause deceleration to a stop.
                    accelTableRepeatsLeft[RA] = 0;
                }
            } 
            
            if (currentSpeed > cmd.stopSpeed[RA]) {
                //If the current speed is now slower than the stopping speed, we can stop moving. So...
                if(gotoRunning(RA)){ 
                    //if we are currently running a goto... 
                    cmd_setGotoEn(RA,CMD_DISABLED); //Switch back to slew mode 
                    clearGotoRunning(RA); //And mark goto status as complete
                } //otherwise don't as it cancels a 'goto ready' state 
                
                cmd_setStopped(RA,CMD_STOPPED); //mark as stopped 
                timerDisable(RA);  //And stop the interrupt timer.
            } 
        } else {
            //If the step pin is currently low...
            setPinValue(stepPin[RA],HIGH); //Set it high to start next step.
            
            //If the current speed is not the target speed, then we are in the accel/decel phase. So...
            byte repeatsReqd = accelTableRepeatsLeft[RA]; //load the number of repeats left for this accel table entry
            if (repeatsReqd == 0) { 
                //If we have done enough repeats for this entry
                unsigned int targetSpeed = cmd.currentIVal[RA]; //Get the target speed
                if (currentSpeed > targetSpeed) {
                    //If we are going too slow
                    byte accelIndex = accelTableIndex[RA]; //Load the acceleration table index
                    if (accelIndex >= AccelTableLength-1) {
                        //If we are at the top of the accel table
                        currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                        accelIndex = AccelTableLength-1; //Ensure index remains in bounds.
                    } else {
                        //Otherwise, we need to accelerate.
                        accelIndex = accelIndex + 1; //Move to the next index
                        accelTableIndex[RA] = accelIndex; //Save the new index back
                        currentSpeed = cmd.accelTable[RA][accelIndex].speed;  //load the new speed from the table
                        if (currentSpeed <= targetSpeed) {
                            //If the new value is too fast
                            currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                        } else {
                            //Load the new number of repeats required
                            if (cmd.highSpeedMode[RA]) {
                                //When in high-speed mode, we need to multiply by sqrt(8) ~= 3 to compensate for the change in steps per rev of the motor
                                accelTableRepeatsLeft[RA] = cmd.accelTable[RA][accelIndex].repeats * 3 + 2;
                            } else {
                                accelTableRepeatsLeft[RA] = cmd.accelTable[RA][accelIndex].repeats;
                            }
                        }
                    }
                } else if (currentSpeed < targetSpeed) {
                    //If we are going too fast
                    byte accelIndex = accelTableIndex[RA]; //Load the acceleration table index
                    if (accelIndex == 0) {
                        //If we are at the bottom of the accel table
                        currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                    } else {
                        //Otherwise, we need to decelerate.
                        accelIndex = accelIndex - 1; //Move to the next index
                        accelTableIndex[RA] = accelIndex; //Save the new index back
                        currentSpeed = cmd.accelTable[RA][accelIndex].speed;  //load the new speed from the table
                        if (currentSpeed >= targetSpeed) {
                            //If the new value is too slow
                            currentSpeed = targetSpeed; //Then the new speed is exactly the target speed.
                        } else {
                            //Load the new number of repeats required
                            if (cmd.highSpeedMode[RA]) {
                                //When in high-speed mode, we need to multiply by sqrt(8) ~= 3 to compensate for the change in steps per rev of the motor
                                accelTableRepeatsLeft[RA] = cmd.accelTable[RA][accelIndex].repeats * 3 + 2;
                            } else {
                                accelTableRepeatsLeft[RA] = cmd.accelTable[RA][accelIndex].repeats;
                            }
                        }
                    }
                }
                currentMotorSpeed(RA, currentSpeed); //Update the current speed in case it has changed.
            } else {
                //Otherwise one more repeat done.
                accelTableRepeatsLeft[RA] = repeatsReqd - 1;
            }
        }
    } else {
        //The required number of interrupts have not yet occurred...
        irqToNextStep(RA, irqToNext); //Update the number of IRQs remaining until the next step.
    }   


}

#else
#error Unsupported Part! Please use an Arduino Mega, or ATMega162
#endif

