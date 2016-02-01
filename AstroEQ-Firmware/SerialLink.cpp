
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


typedef struct {
    unsigned char buffer[64];
    volatile unsigned char head;
    volatile unsigned char tail;
} 
RingBuffer;

RingBuffer txBuf = {{0},0,0};
RingBuffer rxBuf = {{0},0,0};

void Serial_initialise(const unsigned long baud){
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
    rxBuf.head = 0;
    rxBuf.tail = 0;

    //And enable
    sbi(UCSRnB, RXENn);
    sbi(UCSRnB, TXENn);
    sbi(UCSRnB, RXCIEn);
    cbi(UCSRnB, UDRIEn);
}

byte Serial_available(void){
    return ((rxBuf.head - rxBuf.tail) & 0x3F); //number of bytes available
}

char Serial_read(void){
    byte tail = rxBuf.tail;
    if (rxBuf.head == tail) {
        return -1;
    } else {
        char c = rxBuf.buffer[tail];
        rxBuf.tail = ((tail + 1) & 0x3F);
        return c;
    }
}

void Serial_write(char ch){
    unsigned char head = ((txBuf.head + 1) & 0x3F); 
    while (head == txBuf.tail); //wait for buffer to empty
    txBuf.buffer[txBuf.head] = ch;
    txBuf.head = head;

    sbi(UCSRnB, UDRIEn);
}

void Serial_writeStr(char* str){
    while(*str){
        Serial_write(*str++);
    }
}

void Serial_writeArr(char* arr, byte len){
    while(len--){
        Serial_write(*arr++);
    }
}


ISR(USARTn_RX_vect,ISR_NAKED)
{
    register unsigned char c asm("r18");
    register unsigned char head asm("r25");
    register unsigned char tail asm("r24");
    asm volatile (
        "push %0   \n\t"
        :: "a" (c):
    );
    c = SREG;
    asm volatile (
        "push %0   \n\t"
        "push %1   \n\t"
        "push %2   \n\t"
        "push r30  \n\t"
        "push r31  \n\t"
        :: "a" (c), "r" (head), "r" (tail):
    );

    //Read in from the serial data register
    c = UDRn;
    //get the current head
    head = rxBuf.head;
    head++;
    head &= 0x3F;
    tail = rxBuf.tail;

    if (head != tail) {
        rxBuf.buffer[rxBuf.head] = c;
        rxBuf.head = head;
    } 

    asm volatile (
        "pop r31   \n\t"
        "pop r30   \n\t"
        "pop %2   \n\t"
        "pop %1   \n\t"
        "pop %0       \n\t"
        : "=a" (c), "=r" (head), "=r" (tail)::
    );
    SREG = c;
    asm volatile (
        "pop %0       \n\t"
        "reti         \n\t"
        : "=a" (c)::
    );
}

ISR(USARTn_UDRE_vect, ISR_NAKED)
{
    register unsigned char tail asm("r25");
    register unsigned char temp asm("r24");
    asm volatile (
        "push %0       \n\t"
        :: "r" (temp):
    );
    temp = SREG;
    asm volatile (
        "push %0       \n\t"
        "push %1       \n\t"
        "push r30      \n\t"
        "push r31      \n\t"
        :: "r" (temp), "r" (tail):
    );
    tail = txBuf.tail;
    temp = txBuf.head;
    if (temp == tail) {
        // Buffer empty, so disable interrupts
        cbi(UCSRnB, UDRIEn);
    } 
    else {
        // There is more data in the output buffer. Send the next byte
        temp = txBuf.buffer[tail];
        tail++;
        tail &= 0x3F;
        txBuf.tail = tail;
        UDRn = temp;
    }
    asm volatile (
        "pop  r31      \n\t"
        "pop  r30      \n\t"
        "pop  %1       \n\t"
        "pop  %0       \n\t"
        : "=r" (temp), "=r" (tail)::
    );
    SREG = temp;
    asm volatile (
        "pop  %0       \n\t"
        "reti          \n\t"
        : "=r" (temp)::
    );
}

