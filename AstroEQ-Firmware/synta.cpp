
#include "synta.h"
#include "UnionHelpers.h"

void Synta::initialise(unsigned long eVal){
  validPacket = 0;
  _axis = 0;
  commandIndex = 0;
  clearBuffer(commandString,sizeof(commandString));
  //scalar[0] = EEPROM.readByte(scalar1_Address) - 1;
  //scalar[1] = EEPROM.readByte(scalar2_Address) - 1;
  cmd.init(eVal, 1);
}

const char Synta::startInChar = ':';
const char Synta::startOutChar = '=';
const char Synta::errorChar = '!';
const char Synta::endChar = '\r';

void Synta::error(char* buf){
  buf[0] = errorChar;
  buf[1] = endChar;
  buf[2] = 0;
}

void Synta::clearBuffer(char* buf, byte len){
  strncpy(buf,"",len);
}

//void Synta::success(char* buf, char data[], byte dataLen){
//  strcpy(buf + 1,data);
//  buf[0] = startOutChar;
//  buf[dataLen] = endChar;
//  buf[dataLen + 1] = 0;
//}

void Synta::assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData){
  char replyLength = cmd.getLength(commandOrError,0); //get the number of data bytes for response
  
  //char tempStr[11];
  if (replyLength == -1) {
    dataPacket[0] = errorChar;  
  } else {
    dataPacket[0] = startOutChar;
  }
  dataPacket++;
  
  switch (replyLength){
    case -1:
        //error(dataPacket); //In otherwords, invalid command, so send error
        //return;
        break;
    case 0:  
        //clearBuffer(dataPacket+1,sizeof(tempStr)); //empty temporary string
        break;
    case 2:
        byteToHex(dataPacket,responseData);
        break;
    case 3:
        intToHex(dataPacket,responseData);
        break;
    case 6:
        longToHex(dataPacket,responseData);
        break;
  }
  
  dataPacket[(byte)replyLength] = endChar;
  dataPacket[(byte)replyLength + 1] = 0;  
  //success(dataPacket,tempStr,replyLength + 1); //compile response
  return;
}

boolean Synta::validateCommand(byte len){
  _command = commandString[0]; //first byte is command
  _axis = commandString[1] - 49; //second byte is axis
  if(_axis > 1){
    return 0; //incorrect axis
  }
  char requiredLength = cmd.getLength(_command,1); //get the required length of this command
  len -= 3; //Remove the command and axis bytes, aswell as the end char;
  if(requiredLength != len){ //If invalid command, or not required length
    return 0;
  }
  
  byte i;
  for(i = 0;i < len;i++){
    commandString[i] = commandString[i + 2];
  }
  commandString[i] = 0; //Null
  return 1;
}

char Synta::recieveCommand(char* dataPacket, char character){
  if(validPacket){
    if (character == startInChar){
      goto error; //new command without old finishing! (dataPacket contains error message)
    }
    
    commandString[commandIndex++] = character; //Add character to current string build
    
    if(character == endChar){
      if(validateCommand(commandIndex)){
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
    clearBuffer(commandString,sizeof(commandString));
  }
  return 0; //Decode not finished (dataPacket unchanged)
error:
  error(dataPacket);
  validPacket = 0;
  return -1;
}

byte hexToNibbler(char hex) {
  if (hex > '9'){
    hex -= (('A'-'0')-0xA); //even if hex is lower case (e.g. 'a'), the lower nibble will have the correct value as (('a'-'A')&0x0F) = 0.
  }
  return (hex - '0'); //as we are keeping the lower nibble, the -'0' gets optimised away.
}
void hexToByte(char* hex, Nibbler &nibble){
  nibble.low = hexToNibbler(hex[1]);
  nibble.high = hexToNibbler(hex[0]);
}
unsigned long Synta::hexToLong(char* hex){
//  char *boo; //waste point for strtol
//  char str[7]; //Destination of rearranged hex
//  strncpy(str,&hex[4],2); //Lower Byte
//  strncpy(str+2,&hex[2],2); //Middle Byte
//  strncpy(str+4,hex,2); //Upper Byte
//  str[6] = 0;
//  return strtol(str,&boo,16); //convert hex to long integer

  Inter inter = Inter(0); //create a blank inter 
  hexToByte(hex+4,inter.highByter.lowNibbler);
  hexToByte(hex+2,inter.lowByter.highNibbler);
  hexToByte(hex,inter.lowByter.lowNibbler);
  return inter.integer;
}

const char hexLookup[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
void nibbleToHex(char* hex, byte offset, byte nibble) {
//  char hexChar;
//  asm volatile(
//    "movw   Y, %1  \n\t"
//    "add  r28, %2  \n\t"
//    "adc  r29, r1  \n\t"
//    "ld    %0, Y   \n\t"
//    : "=r" (hexChar) //goto selects r18:r21. This adds sts commands for all 4 bytes
//    : "e" (hexLookup),"r" (nibble)       //stepDir is in r19 to match second byte of goto.
//    : "r28","r29"
//  );
//  hex[offset] = hexChar;
  char hexChar = hexLookup[nibble];
  hex[offset] = hexChar;
}
void private_byteToHex(char* hex, Nibbler nibbler){
  nibbleToHex(hex,1, nibbler.low);
  nibbleToHex(hex,0, nibbler.high);
}

void Synta::longToHex(char* hex, unsigned long data){  
  Inter inter = Inter(data);
  private_byteToHex(hex+4,inter.highByter.lowNibbler);
  private_byteToHex(hex+2,inter.lowByter.highNibbler);
  private_byteToHex(hex,inter.lowByter.lowNibbler);
  hex[6] = 0;
}

void Synta::intToHex(char* hex, unsigned int data){
  DoubleNibbler nibble = {data};
  hex[3] = 0;
  nibbleToHex(hex,2, nibble.low);
  nibbleToHex(hex,1, nibble.mid);
  nibbleToHex(hex,0, nibble.high);
}

void Synta::byteToHex(char* hex, byte data){
  Nibbler nibble = {data};
  hex[2] = 0;
  private_byteToHex(hex,nibble);
}

char Synta::command(){
  return _command;
}

byte Synta::axis(byte axis){
  if(axis < 2){
    _axis = axis;
  }
  return _axis;
}
