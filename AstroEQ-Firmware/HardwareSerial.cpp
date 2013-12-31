/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "wiring_private.h"

// this next line disables the entire HardwareSerial.cpp, 
// this is so I can support Attiny series and any other chip without a uart
#if defined(UBRRH) || defined(UBRR0H) || defined(UBRR1H) || defined(UBRR2H) || defined(UBRR3H)

#include "HardwareSerial.h"

// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer (I think), in which head is the index of the location
// to which to write the next incoming character and tail is the index of the
// location from which to read. MAX SIZE IS 128 for this implementation!!
#if (RAMEND < 1000)
  #define SERIAL_BUFFER_SIZE 16
#else
  #define SERIAL_BUFFER_SIZE 96
#endif

template<class T> inline T modulo(T head) { //Not a true '%' function, but it is all that is required by this library!
  if(head >= SERIAL_BUFFER_SIZE){
    head -= SERIAL_BUFFER_SIZE;
  }
  return head;
}

struct ring_buffer
{
  unsigned char buffer[SERIAL_BUFFER_SIZE];
  volatile unsigned char head;
  volatile unsigned char tail;
};

#if defined(USBCON)
  ring_buffer rx_buffer = { { 0 }, 0, 0};
  ring_buffer tx_buffer = { { 0 }, 0, 0};
#endif
#if defined(UBRRH) || defined(UBRR0H)
  ring_buffer rx_buffer  =  { { 0 }, 0, 0 };
  ring_buffer tx_buffer  =  { { 0 }, 0, 0 };
#endif
#if defined(UBRR1H)
  ring_buffer rx_buffer1  =  { { 0 }, 0, 0 };
  ring_buffer tx_buffer1  =  { { 0 }, 0, 0 };
#endif
#if defined(UBRR2H)
  ring_buffer rx_buffer2  =  { { 0 }, 0, 0 };
  ring_buffer tx_buffer2  =  { { 0 }, 0, 0 };
#endif
#if defined(UBRR3H)
  ring_buffer rx_buffer3  =  { { 0 }, 0, 0 };
  ring_buffer tx_buffer3  =  { { 0 }, 0, 0 };
#endif

inline void store_char(unsigned char c, ring_buffer *buffer)
{
  register unsigned char i asm("r25");
  register unsigned char tail asm("r24");
  asm volatile (
    "push %0   \n\t"
    "push %1   \n\t"
    "push r30   \n\t"
    "push r31   \n\t"
    ::"r" (i), "r" (tail):
  );
  i = modulo((unsigned char)(buffer->head+1));// % SERIAL_BUFFER_SIZE;
  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  tail = buffer->tail;
  if (i != tail) {
    buffer->buffer[buffer->head] = c;
    buffer->head = i;
  }
  asm volatile (
    "pop r31   \n\t"
    "pop r30   \n\t"
    "pop %1   \n\t"
    "pop %0   \n\t"
    :"=r" (i), "=r" (tail)::
  );
}

#if !defined(USART0_RX_vect) && defined(USART1_RX_vect)
// do nothing - on the 32u4 the first USART is USART1
#else
#if !defined(USART_RX_vect) && !defined(SIG_USART0_RECV) && \
    !defined(SIG_UART0_RECV) && !defined(USART0_RX_vect) && \
	!defined(SIG_UART_RECV)
  #error "Don't know what the Data Received vector is called for the first UART"
#else
  void serialEvent() __attribute__((weak));
  void serialEvent() {}
  #define serialEvent_implemented
#if defined(USART_RX_vect)
  ISR(USART_RX_vect,ISR_NAKED)
#elif defined(SIG_USART0_RECV)
  ISR(SIG_USART0_RECV,ISR_NAKED)
#elif defined(SIG_UART0_RECV)
  ISR(SIG_UART0_RECV,ISR_NAKED)
#elif defined(USART0_RX_vect)
  ISR(USART0_RX_vect,ISR_NAKED)
#elif defined(SIG_UART_RECV)
  ISR(SIG_UART_RECV,ISR_NAKED)
#endif
  {
    register unsigned char c asm("r18");
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = SREG;
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
  #if defined(UDR0)
    c =  UDR0;
  #elif defined(UDR)
    c =  UDR;
  #else
    #error UDR not defined
  #endif
    store_char(c, &rx_buffer);
    asm volatile (
      "pop %0       \n\t"
      : "=a" (c)::
    );
    SREG = c;
    asm volatile (
      "pop %0       \n\t"
      "reti         \n\t"
      : "=a" (c)::
    );
  }
#endif
#endif

#if defined(USART1_RX_vect)
  void serialEvent1() __attribute__((weak));
  void serialEvent1() {}
  #define serialEvent1_implemented
  ISR(USART1_RX_vect,ISR_NAKED)
  {
    register unsigned char c asm("r18");
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = SREG;
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = UDR1;
    store_char(c, &rx_buffer1);
    asm volatile (
      "pop %0       \n\t"
      : "=a" (c)::
    );
    SREG = c;
    asm volatile (
      "pop %0       \n\t"
      "reti         \n\t"
      : "=a" (c)::
    );
  }
#elif defined(SIG_USART1_RECV)
  #error SIG_USART1_RECV
#endif

#if defined(USART2_RX_vect) && defined(UDR2)
  void serialEvent2() __attribute__((weak));
  void serialEvent2() {}
  #define serialEvent2_implemented
  ISR(USART2_RX_vect,ISR_NAKED)
  {
    register unsigned char c asm("r18");
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = SREG;
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = UDR2;
    store_char(c, &rx_buffer2);
    asm volatile (
      "pop %0       \n\t"
      : "=a" (c)::
    );
    SREG = c;
    asm volatile (
      "pop %0       \n\t"
      "reti         \n\t"
      : "=a" (c)::
    );
  }
#elif defined(SIG_USART2_RECV)
  #error SIG_USART2_RECV
#endif

#if defined(USART3_RX_vect) && defined(UDR3)
  void serialEvent3() __attribute__((weak));
  void serialEvent3() {}
  #define serialEvent3_implemented
  ISR(USART3_RX_vect,ISR_NAKED)
  {
    register unsigned char c asm("r18");
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = SREG;
    asm volatile (
      "push %0       \n\t"
      :: "a" (c):
    );
    c = UDR3;
    store_char(c, &rx_buffer3);
    asm volatile (
      "pop %0       \n\t"
      : "=a" (c)::
    );
    SREG = c;
    asm volatile (
      "pop %0       \n\t"
      "reti         \n\t"
      : "=a" (c)::
    );
  }
#elif defined(SIG_USART3_RECV)
  #error SIG_USART3_RECV
#endif

void serialEventRun(void)
{
#ifdef serialEvent_implemented
  if (Serial.available()) serialEvent();
#endif
#ifdef serialEvent1_implemented
  if (Serial1.available()) serialEvent1();
#endif
#ifdef serialEvent2_implemented
  if (Serial2.available()) serialEvent2();
#endif
#ifdef serialEvent3_implemented
  if (Serial3.available()) serialEvent3();
#endif
}


#if !defined(USART0_UDRE_vect) && defined(USART1_UDRE_vect)
// do nothing - on the 32u4 the first USART is USART1
#else
#if !defined(UART0_UDRE_vect) && !defined(UART_UDRE_vect) && !defined(USART0_UDRE_vect) && !defined(USART_UDRE_vect)
  #error "Don't know what the Data Register Empty vector is called for the first UART"
#else
#if defined(UART0_UDRE_vect)
ISR(UART0_UDRE_vect, ISR_NAKED)
#elif defined(UART_UDRE_vect)
ISR(UART_UDRE_vect, ISR_NAKED)
#elif defined(USART0_UDRE_vect)
ISR(USART0_UDRE_vect, ISR_NAKED)
#elif defined(USART_UDRE_vect)
ISR(USART_UDRE_vect, ISR_NAKED)
#endif
{
  register unsigned char tail asm("r25");
  register unsigned char head asm("r24");
  register unsigned char ch asm("r18");
  asm volatile (
    "push %0       \n\t"
    :: "r" (tail):
  );
  tail = SREG;
  asm volatile (
    "push %0       \n\t"
    "push %1       \n\t"
    "push %2       \n\t"
    "push r30       \n\t"
    "push r31       \n\t"
    :: "r" (tail), "r" (head), "r" (ch):
  );
  tail = tx_buffer.tail;
  head = tx_buffer.head;
  if (head == tail) {
	// Buffer empty, so disable interrupts
#if defined(UCSR0B)
    cbi(UCSR0B, UDRIE0);
#else
    cbi(UCSRB, UDRIE);
#endif
  }
  else {
    // There is more data in the output buffer. Send the next byte
    ch = tx_buffer.buffer[tail];
    tx_buffer.tail = modulo(++tail);
    //tx_buffer.tail = (tx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
	
  #if defined(UDR0)
    UDR0 = ch;
  #elif defined(UDR)
    UDR = ch;
  #else
    #error UDR not defined
  #endif
  }
  asm volatile (
    "pop  r31       \n\t"
    "pop  r30       \n\t"
    "pop  %2       \n\t"
    "pop  %1       \n\t"
    "pop  %0       \n\t"
    :"=r" (tail), "=r" (head), "=r" (ch)::
  );
  SREG = tail;
  asm volatile (
    "pop  %0       \n\t"
    "reti          \n\t"
    :"=r" (tail)::
  );
}
#endif
#endif

#ifdef USART1_UDRE_vect
ISR(USART1_UDRE_vect, ISR_NAKED)
{
  register unsigned char tail asm("r25");
  register unsigned char head asm("r24");
  register unsigned char ch asm("r18");
  asm volatile (
    "push %0       \n\t"
    :: "r" (tail):
  );
  tail = SREG;
  asm volatile (
    "push %0       \n\t"
    "push %1       \n\t"
    "push %2       \n\t"
    "push r30       \n\t"
    "push r31       \n\t"
    :: "r" (tail), "r" (head), "r" (ch):
  );
  tail = tx_buffer1.tail;
  head = tx_buffer1.head;
  if (head == tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR1B, UDRIE1);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    head = tx_buffer1.buffer[tail];
    tx_buffer1.tail = modulo(++tail);
    //tx_buffer1.tail = (tx_buffer1.tail + 1) % SERIAL_BUFFER_SIZE;
	
    UDR1 = head;
  }
  asm volatile (
    "pop  r31       \n\t"
    "pop  r30       \n\t"
    "pop  %2       \n\t"
    "pop  %1       \n\t"
    "pop  %0       \n\t"
    :"=r" (tail), "=r" (head), "=r" (ch)::
  );
  SREG = tail;
  asm volatile (
    "pop  %0       \n\t"
    "reti          \n\t"
    :"=r" (tail)::
  );
}
#endif

#ifdef USART2_UDRE_vect
ISR(USART2_UDRE_vect, ISR_NAKED)
{
  register unsigned char tail asm("r25");
  register unsigned char head asm("r24");
  register unsigned char ch asm("r18");
  asm volatile (
    "push %0       \n\t"
    :: "r" (tail):
  );
  tail = SREG;
  asm volatile (
    "push %0       \n\t"
    "push %1       \n\t"
    "push %2       \n\t"
    "push r30       \n\t"
    "push r31       \n\t"
    :: "r" (tail), "r" (head), "r" (ch):
  );
  tail = tx_buffer2.tail;
  head = tx_buffer2.head;
  if (head == tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR2B, UDRIE2);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    head = tx_buffer2.buffer[tail];
    tx_buffer2.tail = modulo(++tail);
    //tx_buffer2.tail = (tx_buffer2.tail + 1) % SERIAL_BUFFER_SIZE;
	
    UDR2 = head;
  }
  asm volatile (
    "pop  r31       \n\t"
    "pop  r30       \n\t"
    "pop  %2       \n\t"
    "pop  %1       \n\t"
    "pop  %0       \n\t"
    :"=r" (tail), "=r" (head), "=r" (ch)::
  );
  SREG = tail;
  asm volatile (
    "pop  %0       \n\t"
    "reti          \n\t"
    :"=r" (tail)::
  );
}
#endif

#ifdef USART3_UDRE_vect
ISR(USART3_UDRE_vect, ISR_NAKED)
{
  register unsigned char tail asm("r25");
  register unsigned char head asm("r24");
  register unsigned char ch asm("r18");
  asm volatile (
    "push %0       \n\t"
    :: "r" (tail):
  );
  tail = SREG;
  asm volatile (
    "push %0       \n\t"
    "push %1       \n\t"
    "push %2       \n\t"
    "push r30       \n\t"
    "push r31       \n\t"
    :: "r" (tail), "r" (head), "r" (ch):
  );
  tail = tx_buffer3.tail;
  head = tx_buffer3.head;
  if (head == tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR3B, UDRIE3);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    head = tx_buffer3.buffer[tail];
    tx_buffer3.tail = modulo(++tail);
    //tx_buffer3.tail = (tx_buffer3.tail + 1) % SERIAL_BUFFER_SIZE;
	
    UDR3 = head;
  }
  asm volatile (
    "pop  r31       \n\t"
    "pop  r30       \n\t"
    "pop  %2       \n\t"
    "pop  %1       \n\t"
    "pop  %0       \n\t"
    :"=r" (tail), "=r" (head), "=r" (ch)::
  );
  SREG = tail;
  asm volatile (
    "pop  %0       \n\t"
    "reti          \n\t"
    :"=r" (tail)::
  );
}
#endif


// Constructors ////////////////////////////////////////////////////////////////

HardwareSerial::HardwareSerial(ring_buffer *rx_buffer, ring_buffer *tx_buffer,
  volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
  volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
  volatile uint8_t *udr,
  uint8_t rxen, uint8_t txen, uint8_t rxcie, uint8_t udrie, uint8_t u2x)
{
  _rx_buffer = rx_buffer;
  _tx_buffer = tx_buffer;
  _ubrrh = ubrrh;
  _ubrrl = ubrrl;
  _ucsra = ucsra;
  _ucsrb = ucsrb;
  _udr = udr;
  _rxen = rxen;
  _txen = txen;
  _rxcie = rxcie;
  _udrie = udrie;
  _u2x = u2x;
}

// Public Methods //////////////////////////////////////////////////////////////

void HardwareSerial::begin(unsigned long baud)
{
  uint16_t baud_setting;
  bool use_u2x = true;

#if (F_CPU == 16000000UL) || (F_CPU == 24000000UL)
  // hardcoded exception for compatibility with the bootloader shipped
  // with the Duemilanove and previous boards and the firmware on the 8U2
  // on the Uno and Mega 2560.
  if (baud == 57600) {
    use_u2x = false;
  }
#endif

try_again:
  
  if (use_u2x) {
    *_ucsra = 1 << _u2x;
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  } else {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }
  
  if ((baud_setting > 4095) && use_u2x)
  {
    use_u2x = false;
    goto try_again;
  }

  // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  sbi(*_ucsrb, _rxen);
  sbi(*_ucsrb, _txen);
  sbi(*_ucsrb, _rxcie);
  cbi(*_ucsrb, _udrie);
}

void HardwareSerial::end()
{
  // wait for transmission of outgoing data
  while (_tx_buffer->head != _tx_buffer->tail)
    ;

  cbi(*_ucsrb, _rxen);
  cbi(*_ucsrb, _txen);
  cbi(*_ucsrb, _rxcie);  
  cbi(*_ucsrb, _udrie);
  
  // clear any received data
  _rx_buffer->head = _rx_buffer->tail;
}

int HardwareSerial::available(void)
{
  unsigned int difference = SERIAL_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail;
  return modulo(difference);
  //return (unsigned int)(SERIAL_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

int HardwareSerial::peek(void)
{
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    return _rx_buffer->buffer[_rx_buffer->tail];
  }
}

int HardwareSerial::read(void)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
    _rx_buffer->tail = modulo(++_rx_buffer->tail);//(unsigned int)(_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
    return c;
  }
}

void HardwareSerial::flush()
{
  while (_tx_buffer->head != _tx_buffer->tail)
    ;
}

size_t HardwareSerial::write(uint8_t c)
{
  unsigned char i = modulo((unsigned char)(_tx_buffer->head+1));//(unsigned char)((_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE);
	
  // If the output buffer is full, there's nothing for it other than to 
  // wait for the interrupt handler to empty it a bit
  // ???: return 0 here instead?
  while (i == _tx_buffer->tail)
    ;
	
  _tx_buffer->buffer[_tx_buffer->head] = c;
  _tx_buffer->head = i;
	
  sbi(*_ucsrb, _udrie);
  
  return 1;
}

HardwareSerial::operator bool() {
	return true;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

#if defined(UBRRH) && defined(UBRRL)
  HardwareSerial Serial(&rx_buffer, &tx_buffer, &UBRRH, &UBRRL, &UCSRA, &UCSRB, &UDR, RXEN, TXEN, RXCIE, UDRIE, U2X);
#elif defined(UBRR0H) && defined(UBRR0L)
  HardwareSerial Serial(&rx_buffer, &tx_buffer, &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UDR0, RXEN0, TXEN0, RXCIE0, UDRIE0, U2X0);
#elif defined(USBCON)
  // do nothing - Serial object and buffers are initialized in CDC code
#else
  #error no serial port defined  (port 0)
#endif

#if defined(UBRR1H)
  HardwareSerial Serial1(&rx_buffer1, &tx_buffer1, &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UDR1, RXEN1, TXEN1, RXCIE1, UDRIE1, U2X1);
#endif
#if defined(UBRR2H)
  HardwareSerial Serial2(&rx_buffer2, &tx_buffer2, &UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UDR2, RXEN2, TXEN2, RXCIE2, UDRIE2, U2X2);
#endif
#if defined(UBRR3H)
  HardwareSerial Serial3(&rx_buffer3, &tx_buffer3, &UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UDR3, RXEN3, TXEN3, RXCIE3, UDRIE3, U2X3);
#endif

#endif // whole file

