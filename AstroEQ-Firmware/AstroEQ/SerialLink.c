
#include "SerialLink.h"
#include <avr/io.h>
#include <avr/interrupt.h>

//This stuff is needed for GCC to correctly append in the value from SERIALn above
#define __REGnD(r,n,d) r##n##d
#define __REGn(r,n) r##n
#define _REGnD(r,n,d) __REGnD(r,n,d)
#define _REGn(r,n) __REGn(r,n)

//Two macros to make things like UCSRA0A and U2X0
#define REGnD(r,d) _REGnD(r,SERIALn,d)
#define REGn(r) _REGn(r,SERIALn)

//Registers
#define UCSRnA REGnD(UCSR,A)
#define UCSRnB REGnD(UCSR,B)
#define UBRRnH REGnD(UBRR,H)
#define UBRRnL REGnD(UBRR,L)
#define UDRn REGn(UDR)

//Bits
#define U2Xn REGn(U2X)
#define RXENn REGn(RXEN)
#define TXENn REGn(TXEN)
#define RXCIEn REGn(RXCIE)
#define UDRIEn REGn(UDRIE)

//Vectors
#define USARTn_RX_vect REGnD(USART,_RX_vect)
#define USARTn_UDRE_vect REGnD(USART,_UDRE_vect)

#define SPI_NULL 0xFF //NULL means ignore
#define SPI_DATA 0x7F //And-ed with all outgoing write packets. An incoming byte is ignored if bits outside this mask are set.
#define SPI_READ 0x81 //SPI read request command
#define SPI_RESP 0x80 //SPI read response request command
#define SPI_ISDATA(a) (!((a) & (~SPI_DATA))) //Returns true if SPI_DATA
#define SPI_ISREAD(a) ( ((a) == ( SPI_READ))) //Returns true if SPI_READ

#define BUFFER_SIZE 32 //Must be power of 2!
#define BUFFER_PTR_MASK (BUFFER_SIZE - 1)
typedef struct {
    unsigned char buffer[BUFFER_SIZE];
    volatile unsigned char head;
    volatile unsigned char tail;
} 
RingBuffer;

RingBuffer txBuf = {{0},0,0};
RingBuffer rxBuf = {{0},0,0};

bool softSPIEnabled = false;

//Initialise the hardware UART port and set baud rate.
void Serial_initialise(const unsigned long baud) {
    Byter baud_setting;

    UCSRnA = _BV(U2Xn);
    baud_setting.integer = (F_CPU / 4 / baud - 1) / 2;

    if (baud_setting.high & 0xF0) {
        UCSRnA = 0;
        baud_setting.integer = (F_CPU / 8 / baud - 1) / 2;
    }

    // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
    UBRRnH = baud_setting.high & 0x0F;
    UBRRnL = baud_setting.low;

    //Drain the serial port of anything that might be in the buffer
    Serial_clear(); //Empty the buffer of any outstanding data.

    //And enable
    sbi(UCSRnB, RXENn);
    sbi(UCSRnB, TXENn);
    sbi(UCSRnB, RXCIEn);
    cbi(UCSRnB, UDRIEn);
}

//Disable the hardware UART port
void Serial_disable() {
    cbi(UCSRnB, RXENn);
    cbi(UCSRnB, TXENn);
    cbi(UCSRnB, RXCIEn);
    cbi(UCSRnB, UDRIEn);
    Serial_clear(); //Empty the buffer of any outstanding data.
}

//Clear the serial buffers
void Serial_clear(void) {
    byte oldSREG = SREG;
    cli();
    //Clear the buffers by resetting the heads and tails to be equal.
    txBuf.head = 0;
    txBuf.tail = 0;
    rxBuf.head = 0;
    rxBuf.tail = 0;
    SREG = oldSREG;
}

//Initialise the Software SPI by setting ports to correct direction and state.
void SPI_initialise() {
    //Set all SPI pins to idle levels
    setPinDir  (SPIClockPin_Define,OUTPUT); //Clock is output idle high
    setPinValue(SPIClockPin_Define,  HIGH);
    setPinDir  (SPIMISOPin_Define,  INPUT); //MISO is input pull-up
    setPinValue(SPIMISOPin_Define,   HIGH);
    setPinDir  (SPIMOSIPin_Define, OUTPUT); //MOSI is output idle high
    setPinValue(SPIMOSIPin_Define,   HIGH);
    setPinDir  (SPISSnPin_Define,  OUTPUT); //SSn is output idle high
    setPinValue(SPISSnPin_Define,    HIGH);
    //Standalone pin is switching to SPI ready, so ensure we out pull-up is to high.
    setPinValue(standalonePin[STANDALONE_PULL],HIGH); //Pull high
    //Drain the serial port of anything that might be in the buffer
    Serial_clear(); //Empty the buffer of any outstanding data.
    
    //Now enabled
    softSPIEnabled = true;
}

//Disable the Software SPI by setting all ports back to input pull-up
void SPI_disable() {
    //Set all SPI pins to High-Z
    setPinDir  (SPIClockPin_Define, INPUT);
    setPinValue(SPIClockPin_Define,  HIGH);
    setPinDir  (SPIMISOPin_Define,  INPUT);
    setPinValue(SPIMISOPin_Define,   HIGH);
    setPinDir  (SPIMOSIPin_Define,  INPUT);
    setPinValue(SPIMOSIPin_Define,   HIGH);
    setPinDir  (SPISSnPin_Define,   INPUT);
    setPinValue(SPISSnPin_Define,    HIGH);
    //Now disabled
    softSPIEnabled = false;
    Serial_clear(); //Empty the buffer of any outstanding data.
}

//Software SPI Mode 3 Transfer
//One byte of data is sent and at the same time a byte is received.
byte SPI_transfer(byte data) {
    for (byte i = 8;i > 0; i--){ //Count through all 8 bits.
        setPinValue(SPIClockPin_Define,LOW); //Falling Edge   //--     2 cycles         .
        if (data & 0x80) { //Send MSB first                   //-- 2 cycles | 1 cycle    |
            setPinValue(SPIMOSIPin_Define,HIGH);              //-- 2 cycles | -          |
        } else {                                              //--          | 2 cycles    > 8 cycles for both paths
            nop();                                            //--          | 1 cycle    |
            setPinValue(SPIMOSIPin_Define,LOW);               //--          | 2 cycles   |
        }                                                     //-- 2 cycles | -         '
        setPinValue(SPIClockPin_Define,HIGH); //Rising Edge   //--      2 cycles        .
        data = data << 1; //Shift MSB-1 to MSB                //--      1 cycle          |
        if (getPinValue(SPIMISOPin_Define)) {                 //-- 2 cycle  | 1 cycles    > 8 cycles for both paths
            data = data + 1;//and set LSB to the new data     //--          | 1 cycle    |
        }                                                     //--       3 cycles       '
    }                                                         //-- Total Path is 16 cycles = 1MHz @ 16MHz clock
    return data; //Return shifted in data.                    //-- 5 Cycles on entry (including CALL), 3 cycles on exit (including RET)
}

//Performs an SPI read request and stores the data in the RX buffer.
// - If there is no space in the buffer, a read request will *not* be performed
//   The buffer should be first emptied by using Serial_read()
void SPI_read(void) {
    //First we check if there is space in the buffer, and that the slave has data to send
    if ((rxBuf.tail != rxBuf.head) && !(getPinValue(standalonePin[STANDALONE_IRQ]))) {
        //If there is, then do a read request  
        setPinValue(SPISSnPin_Define,LOW); //Select the slave
        SPI_transfer(SPI_READ); //First send a read request
        while(!getPinValue(standalonePin[STANDALONE_IRQ])); //Wait for the slave to have loaded its data
        byte data = SPI_transfer(SPI_RESP); //Then send a response request (clocks data from slave to master and informs slave that transfer is done)
        if (SPI_ISDATA(data)) {
            //If the slave had data available (indicated by the MSB being clear)
            rxBuf.buffer[rxBuf.head] = data; //Store the data
            rxBuf.head++; //And increment the head
        }
        setPinValue(SPISSnPin_Define,HIGH); //Deselect the slave
    }
}

//Performs an SPI write request.
void SPI_write(byte data) {
    setPinValue(SPISSnPin_Define,LOW); //Select the slave
    SPI_transfer(data & SPI_DATA); 
    setPinValue(SPISSnPin_Define,HIGH); //Deselect the slave
}

//Checks if there is any data available in the RX buffer.
// - If in SPI mode, this will also perform an SPI read transfer to see if there is any valid data.
byte Serial_available(void) {
    if (softSPIEnabled) {
        //If SPI is enabled, we do a read to check if there is any data.
        SPI_read();
    }
    return ((rxBuf.head - rxBuf.tail) & BUFFER_PTR_MASK); //number of bytes available
}

//Returns the next available data byte in the buffer
// - If there is nothing there, -1 is returned.
char Serial_read(void) {
    //If UART is enabled
    byte tail = rxBuf.tail;
    if (rxBuf.head == tail) {
        return -1;
    } else {
        char c = rxBuf.buffer[tail];
        rxBuf.tail = ((tail + 1) & BUFFER_PTR_MASK);
        return c;
    }
}

//Write a byte of data
// - If in UART mode, the byte is stored into the TX buffer when there is space.
// - If in SPI mode, a write transfer is performed.
void Serial_write(char ch) {
    if (UCSRnB & _BV(TXENn)) { 
        //If UART is enabled
        unsigned char head = ((txBuf.head + 1) & BUFFER_PTR_MASK); //Calculate the new head
        if (head == txBuf.tail) {
            //If there is no space in the buffer
            sbi(UCSRnB, UDRIEn); //Ensure TX IRQ is enabled before our busy wait - otherwise we lock up!
            while (head == txBuf.tail); //wait for buffer to have some space
        }
        
        txBuf.buffer[txBuf.head] = ch; //Load the new data into the buffer
        txBuf.head = head; //And store the new head.
        sbi(UCSRnB, UDRIEn); //Ensure TX IRQ is enabled if not already
    } else if (softSPIEnabled) {
        //If SPI is enabled, we do an SPI write.
        SPI_write(ch);
    }
}

//Flushes data from TX buffer
void Serial_flush() {
    if (UCSRnB & _BV(TXENn)) { 
        //If UART is enabled
        unsigned char head = txBuf.head; //Calculate the new head
        if (head != txBuf.tail) {
            //If there is still data to be sent
            sbi(UCSRnB, UDRIEn); //Ensure TX IRQ is enabled before our busy wait - otherwise we lock up!
            while (head != txBuf.tail); //wait for buffer to empty
        }
    }
}

//Convert string to bytes
void Serial_writeStr(char* str) {
    while (*str) {
        Serial_write(*str++);
    }
}

//Convert array to bytes
void Serial_writeArr(char* arr, byte len) {
    while (len--) {
        Serial_write(*arr++);
    }
}

//UART RX IRQ
// - Stores data from UART port into RX ring buffer
ISR(USARTn_RX_vect,ISR_NAKED) {
    register unsigned char c asm("r18");
    register unsigned char head asm("r25");
    register unsigned char tail asm("r24");
    asm volatile (
        "push %0     \n\t"
        "in   %0, %3 \n\t" 
        "push %0     \n\t"
        "push %1     \n\t"
        "push %2     \n\t"
        "push r30    \n\t"
        "push r31    \n\t"
        :
        : "a" (c), "r" (head), "r" (tail), "I" (_SFR_IO_ADDR(SREG))
        :
    );

    //Read in from the serial data register
    c = UDRn;
    //get the current head
    head = rxBuf.head;
    head++;
    head &= BUFFER_PTR_MASK;
    tail = rxBuf.tail;

    if (head != tail) {
        rxBuf.buffer[rxBuf.head] = c;
        rxBuf.head = head;
    } 

    asm volatile (
        "pop r31    \n\t"
        "pop r30    \n\t"
        "pop %2     \n\t"
        "pop %1     \n\t"
        "pop %0     \n\t"
        "out %3, %0 \n\t"
        "pop %0     \n\t"
        "reti       \n\t"
        : "=a" (c), "=r" (head), "=r" (tail) 
        : "I" (_SFR_IO_ADDR(SREG)) 
        :
    );
}

//UART TX IRQ
// - Writes data to UART port from TX ring buffer
ISR(USARTn_UDRE_vect, ISR_NAKED)
{
    register unsigned char tail asm("r25");
    register unsigned char temp asm("r24");
    asm volatile (
        "push %0       \n\t"
        "in   %0, %2   \n\t" 
        "push %0       \n\t"
        "push %1       \n\t"
        "push r30      \n\t"
        "push r31      \n\t"
        :: "r" (temp), "r" (tail), "I" (_SFR_IO_ADDR(SREG)):
    );
    tail = txBuf.tail;
    temp = txBuf.head;
    if (temp == tail) {
        // Buffer empty, so disable interrupts
        cbi(UCSRnB, UDRIEn);
    } else {
        // There is more data in the output buffer. Send the next byte
        temp = txBuf.buffer[tail];
        tail++;
        tail &= BUFFER_PTR_MASK;
        txBuf.tail = tail;
        UDRn = temp;
    }

    asm volatile (
        "pop r31    \n\t"
        "pop r30    \n\t"
        "pop %1     \n\t"
        "pop %0     \n\t"
        "out %2, %0 \n\t"
        "pop %0     \n\t"
        "reti       \n\t"
        : "=r" (temp), "=r" (tail) 
        : "I" (_SFR_IO_ADDR(SREG)) 
        :
    );
}
