
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
    
      static Synta& getInstance(unsigned long version)
      {
        static Synta singleton = Synta(version);
        return singleton;
      }
      
      Commands cmd;
      void assembleResponse(char* dataPacket, char commandOrError, unsigned long responseData);
      char recieveCommand(char* dataPacket, char character);
      byte axis(byte axis = 2); //make target readonly to outside world.
      char command(); //make current command readonly to outside world.
      
      unsigned long hexToLong(char* hex);
      void longToHex(char* hex, unsigned long data);
      void intToHex(char* hex, unsigned int data);
      void byteToHex(char* hex, byte data);
      
      //byte scalar[2];
    
    private:
      Synta(unsigned long version) {
        initialise(version);
      };
      //Synta(Synta const&);
      void operator=(Synta const&);
      
      void initialise(unsigned long eVal);
      
      
      void clearBuffer(char* buf, byte len);
//      void success(char* buf, char data[], byte dataLen);
      void error(char* buf);
      boolean validateCommand(byte len);
      
      boolean validPacket;
      char commandString[11];
      byte commandIndex;
      
      byte _axis;
      char _command;
      
      static const char startInChar;
      static const char startOutChar;
      static const char errorChar;
      static const char endChar;
  };
#endif
