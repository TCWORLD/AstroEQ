
#include "synta.h"
#include "commands.h"
#include <string.h>


bool validateCommand(byte len);
bool validPacket;
char commandString[11];
byte commandIndex;

byte _axis;
char _command;

void synta_initialise(unsigned long eVal, byte gVal){
    validPacket = 0;
    commandIndex = 0;
    memset(commandString,0,sizeof(commandString));
    _axis = 0;
    Commands_init(eVal, gVal);
}

const char startInChar = ':';
const char startOutChar = '=';
const char errorChar = '!';
const char endChar = '\r';

inline void nibbleToHex(char* hex, byte nibble) {
    if (nibble > 9){
        nibble += (('A'-'0')-0xA);
    }
    *hex = (nibble + '0');
}

inline void private_byteToHex(char* lower, char* upper, Nibbler nibbler){
    nibbleToHex(lower, nibbler.low);
    nibbleToHex(upper, nibbler.high);
}

void synta_assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData, bool isProg) {
    char replyLength = (commandOrError == '\0') ? -1 : Commands_getLength(commandOrError, CMD_LEN_SEND, isProg); //get the number of data bytes for response

    if (replyLength < 0) {
        byte errorCode = responseData;
        replyLength = 2;
        dataPacket[0] = errorChar;
        dataPacket[1] = '0';
        nibbleToHex(dataPacket+2, errorCode & 0xF);
    } else {
        dataPacket[0] = startOutChar;

        if (replyLength == 2) {
            Nibbler nibble = { responseData };
            private_byteToHex(dataPacket+2,dataPacket+1,nibble);
        } else if (replyLength == 3) {
            DoubleNibbler nibble = { responseData };
            nibbleToHex(dataPacket+3, nibble.low);
            nibbleToHex(dataPacket+2, nibble.mid);
            nibbleToHex(dataPacket+1, nibble.high);
        } else if (replyLength == 6) {
            Inter inter = { responseData };
            private_byteToHex(dataPacket+6,dataPacket+5,inter.highByter.lowNibbler);
            private_byteToHex(dataPacket+4,dataPacket+3,inter.lowByter.highNibbler);
            private_byteToHex(dataPacket+2,dataPacket+1,inter.lowByter.lowNibbler);
        }
    }
    dataPacket = dataPacket + (size_t)replyLength;
    dataPacket[1] = endChar;
    dataPacket[2] = '\0';
    return;
}

// Returns negative for success, otherwise an error code
char synta_validateCommand(byte len, char* decoded, bool isProg){
    _command = commandString[0]; //first byte is command
    _axis = commandString[1] - '1'; //second byte is axis
    if(_axis > 1){
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
    return -1; //No error
}

char synta_recieveCommand(char* dataPacket, char character, bool isProg){
    char status = -1;
    char errorCode;
    if(validPacket){
        if (character == startInChar){
            status = -2;
            errorCode = SYNTA_ERROR_INVALIDCHAR;
            goto error;
        }

        commandString[commandIndex++] = character; //Add character to current string build

        if(character == endChar){
            if((errorCode = synta_validateCommand(commandIndex, dataPacket, isProg)) < 0){
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
    return 0; //Decode not finished (dataPacket unchanged)
error:
    dataPacket[0] = errorChar;
    dataPacket[1] = '0';
    nibbleToHex(dataPacket+2, errorCode & 0xF);
    dataPacket[3] = '\0';
    dataPacket[4] = endChar;
    validPacket = 0;
    return status;
}

inline byte hexToNibbler(char hex) {
    if (hex > '9'){
        hex -= (('A'-'0')-0xA); //even if hex is lower case (e.g. 'a'), the lower nibble will have the correct value as (('a'-'A')&0x0F) = 0.
    }
    return (hex - '0'); //as we are keeping the lower nibble, the -'0' gets optimised away.
}
inline byte hexToByte(char* hex){
    //nibble.low = hexToNibbler(hex[1]);
    //nibble.high = hexToNibbler(hex[0]);
    Nibbler low = {hexToNibbler(hex[1])};
    Nibbler high = {hexToNibbler(hex[0])<<4};
    return ((high.high<<4)|low.low);
}

byte synta_hexToByte(char* hex){
    return hexToByte(hex);
}
unsigned long synta_hexToLong(char* hex){
    //  char *boo; //waste point for strtol
    //  char str[7]; //Destination of rearranged hex
    //  strncpy(str,&hex[4],2); //Lower Byte
    //  strncpy(str+2,&hex[2],2); //Middle Byte
    //  strncpy(str+4,hex,2); //Upper Byte
    //  str[6] = 0;
    //  return strtol(str,&boo,16); //convert hex to long integer

    Inter inter = InterMaker(0,hexToByte(hex+4),hexToByte(hex+2),hexToByte(hex)); //create an inter 
    return inter.integer; //and convert it to an integer
}

char synta_command(){
    return _command;
}

byte synta_setaxis(byte axis){
	if(axis < 2){
		_axis = axis;
	}
	return _axis;
}

byte synta_getaxis(){
	return _axis;
}

