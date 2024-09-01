
#include "synta.h"

#include <avr/builtins.h>
#include <string.h>

static bool validPacket;
static char commandString[11];
static byte commandIndex;

static MotorAxis _axis;
static char _command;

//Axis conversion
static inline MotorAxis mapAxis(char axis) {
    return (MotorAxis)(axis - '1');
}

void synta_initialise(unsigned long eVal, byte gVal){
    validPacket = 0;
    commandIndex = 0;
    memset(commandString,0,sizeof(commandString));
    _axis = RA;
    Commands_init(eVal, gVal);
}

static const char startInChar = ':';
static const char startOutChar = '=';
static const char errorChar = '!';
static const char endChar = '\r';

static void nibbleToHex(char* hex, byte nibble) {
    nibble &= 0xF;
    if (nibble > 9) *hex = nibble + 'A' - 10;
    else *hex = nibble + '0';
}

static void byteToHex(char* hex, byte value){
    nibbleToHex(hex, __builtin_avr_swap(value));
    nibbleToHex(hex+1, value);
}

void synta_assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData, CmdProgMode isProg) {
    char replyLength = (commandOrError == '\0') ? -1 : Commands_getLength(commandOrError, CMD_LEN_SEND, isProg); //get the number of data bytes for response

    if (replyLength < 0) {
        dataPacket[0] = errorChar;
        if (isProg == CMD_LEN_PROG) {
            // Don't use error codes in programming mode
            // as the config utility is not set up to
            // handle them.
            replyLength = 0;
        } else {
            byte errorCode = responseData;
            replyLength = 2;
            dataPacket[1] = '0';
            nibbleToHex(dataPacket+2, errorCode);
        }        
    } else {
        dataPacket[0] = startOutChar;

        if (replyLength == 2) {
            byteToHex(dataPacket+1,responseData);
        } else if (replyLength == 3) {
            nibbleToHex(dataPacket+1, responseData >> 8);
            byteToHex(dataPacket+2, responseData);
        } else if (replyLength == 6) {
            byteToHex(dataPacket+5,responseData >> 16);
            byteToHex(dataPacket+3,responseData >> 8);
            byteToHex(dataPacket+1,responseData);
        }
    }
    dataPacket = dataPacket + (size_t)replyLength;
    dataPacket[1] = endChar;
    dataPacket[2] = '\0';
    return;
}

// Returns negative for success, otherwise an error code
SyntaError synta_validateCommand(byte len, char* decoded, CmdProgMode isProg){
    _command = commandString[0]; //first byte is command
    _axis = mapAxis(commandString[1]); //second byte is axis
    if(_axis > AXIS_COUNT){
        return SYNTA_ERROR_INVALIDCHAR; //incorrect axis
    }
    char requiredLength = Commands_getLength(_command, CMD_LEN_RECV, isProg); //get the required length of this command
    len -= 3; //Remove the command and axis bytes, aswell as the end char;
    if(requiredLength != len){ //If invalid command, or not required length
        return SYNTA_ERROR_CMDLENGTH;
    }
    byte i;
    for(i = 0;i < len;i++){
        decoded[i] = commandString[i + 2];
    }
    decoded[i] = '\0'; //Null
    return SYNTA_NOERROR; //No error
}

char synta_recieveCommand(char* dataPacket, char character, CmdProgMode isProg){
    char status = PACKET_ERROR_GENERAL;
    SyntaError errorCode;
    if(validPacket){
        if (character == startInChar){
            status = PACKET_ERROR_BADCHAR;
            errorCode = SYNTA_ERROR_INVALIDCHAR;
            goto error;
        }

        commandString[commandIndex++] = character; //Add character to current string build

        if(character == endChar){
            if((errorCode = synta_validateCommand(commandIndex, dataPacket, isProg)) == SYNTA_NOERROR){
                validPacket = 0;
                return _command; //Successful decode (dataPacket contains decoded packet, return value is the current command)
            } else {
                goto error; //Decode Failed (dataPacket contains error message)
            }
        } else if (commandIndex == sizeof(commandString)){
            errorCode = SYNTA_ERROR_CMDLENGTH;
            goto error; //Message too long! (dataPacket contains error message)
        }
    } else if (character == startInChar){
        //Begin new command
        commandIndex = 0;
        validPacket = 1;
        commandString[0] = '\0';
    }
    return PACKET_ERROR_PARTIAL; //Decode not finished (dataPacket unchanged)
error:
    dataPacket[0] = errorChar;
    dataPacket[1] = '0';
    nibbleToHex(dataPacket+2, errorCode);
    dataPacket[3] = '\0';
    dataPacket[4] = endChar;
    validPacket = 0;
    return status;
}

static inline byte hexToNibble(char hex) {
    if (hex > '9'){
        return (hex - 'A' + 10) & 0xF; //even if hex is lower case (e.g. 'a'), the lower nibble will have the correct value as (('a'-'A')&0x0F) = 0.
    } else {
        return (hex - '0') & 0xF; //as we are keeping the lower nibble, the -'0' gets optimised away.
    }
}

byte synta_hexToByte(char* hex){
    byte asByte = hexToNibble(hex[1]) | __builtin_avr_swap(hexToNibble(hex[0]));
    return asByte;
}

unsigned long synta_hexToLong(char* hex){
    // Convert bytes from little endian hex
    FourBytes pack = {0};
    pack.bytes[0] = synta_hexToByte(hex);
    pack.bytes[1] = synta_hexToByte(hex+2);
    pack.bytes[2] = synta_hexToByte(hex+4);
    return pack.integer;
}

char synta_command(){
    return _command;
}

MotorAxis synta_setaxis(MotorAxis axis){
	if(axis < AXIS_COUNT){
		_axis = axis;
	}
	return _axis;
}

MotorAxis synta_getaxis(){
	return _axis;
}

