
#include "synta.h"

Synta::Synta(unsigned long eVal,unsigned long aVal,unsigned long bVal,byte gVal,unsigned long sVal,byte scalar){
  validPacket = 0;
  _axis = 0;
  commandIndex = 0;
  clearBuffer(commandString,sizeof(commandString));
  _scalar = scalar;
  cmd.init(eVal, aVal, bVal, gVal, sVal);
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

void Synta::success(char* buf, char data[], byte dataLen){
  strcpy(buf + 1,data);
  buf[0] = startOutChar;
  buf[dataLen] = endChar;
  buf[dataLen + 1] = 0;
}

void Synta::assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData){
  char replyLength = cmd.getLength(commandOrError,0); //get the number of data bytes for response
  
  char tempStr[11];
  
  switch (replyLength){
    case -1:
        error(dataPacket); //In otherwords, invalid command, so send error
        return;
    case 0:
        clearBuffer(tempStr,sizeof(tempStr)); //empty temporary string
        break;
    case 2:
        byteToHex(tempStr,responseData);
        break;
    case 3:
        intToHex(tempStr,responseData);
        break;
    case 6:
        longToHex(tempStr,responseData);
        break;
  }        
  success(dataPacket,tempStr,replyLength + 1); //compile response
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

unsigned long Synta::hexToLong(char* hex){
  char *boo; //waste point for strtol
  char str[7]; //Destination of rearranged hex
  strncpy(str,&hex[4],2); //Lower Byte
  strncpy(str+2,&hex[2],2); //Middle Byte
  strncpy(str+4,hex,2); //Upper Byte
  str[6] = 0;
  return strtol(str,&boo,16); //convert hex to long integer
}

void Synta::longToHex(char* hex, unsigned long data){
  byte bytes[3];
  bytes[0] = (data >> 16) & 0xFF;
  bytes[1] = (data >> 8) & 0xFF;
  bytes[2] = (data) & 0xFF;
  sprintf(hex,"%02X%02X%02X",bytes[2],bytes[1],bytes[0]);
}

void Synta::intToHex(char* hex, unsigned int data){
  data &= 0xFFF;
  sprintf(hex,"%03X",data);
}

void Synta::byteToHex(char* hex, byte data){
  data &= 0xFF;
  sprintf(hex,"%02X",data);
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

byte Synta::scalar(){
  return _scalar;
}
