
#ifndef synta_h
#define synta_h
  #if ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
  
  #include "commands.h"
  
  class Synta{
    public:
      Synta(unsigned long eVal,unsigned long aVal,unsigned long bVal,byte gVal,unsigned long sVal,byte scalar);
      Synta(unsigned long eVal,unsigned long aVal1,unsigned long aVal2,unsigned long bVal1,unsigned long bVal2,unsigned long sVal1,unsigned long sVal2,byte gVal,byte scalar);
      Commands cmd;
      void assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData);
      char recieveCommand(char* dataPacket, char character);
      byte axis(byte axis = 2); //make target readonly to outside world.
      byte scalar();
      char command(); //make current command readonly to outside world.
      
      unsigned long hexToLong(char* hex);
      void longToHex(char* hex, unsigned long data);
      void intToHex(char* hex, unsigned int data);
      void byteToHex(char* hex, byte data);
    
    private:
      void clearBuffer(char* buf, byte len);
      void success(char* buf, char data[], byte dataLen);
      void error(char* buf);
      boolean validateCommand(byte len);
      
      boolean validPacket;
      char commandString[11];
      char commandIndex;
      
      byte _axis;
      char _command;
      byte _scalar;
      
      static const char startInChar;
      static const char startOutChar;
      static const char errorChar;
      static const char endChar;
  };
#endif
