/*
   Simple library for Arduino implementing a hardware counter
   for a Geigier counter for example

   Copyright (c) 2011, Robin Scheibler aka FakuFaku
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Counter.h"
#include <limits.h>

// need to have the global variable to count
// overflows
unsigned int g_ovf_n;

// Constructor
HardwareCounter::HardwareCounter(int timer_pin, long delay)
{
  // register delay
  _delay = delay;
  // register timer pin
  _pin = timer_pin;
}

// call this to start the counter
void HardwareCounter::start()
{
  // set pin as digital input
  pinMode(_pin, INPUT);

  // hardware counter setup ( refer atmega168.pdf chapter 16-bit counter1)
  TIMSKn = 0; // disable overflow interrupt
  TCCRnA = 0;     // reset timer/countern control register A
  TCCRnB = 0;     // reset timer/countern control register A

  // Reset the counter to zero
  TCNTn = 0;      // counter register value = 0
  g_ovf_n = 0;    // reset number of overflows
  _count = 0;     // set count to zero (optional)

  // set overflow interrupt
  TIFR1 |= 1;            // clear overflow interrupt flag
  TIMSKn |= _BV(TOIEn);  // enable overflow interrupt

  // set start time
  _start_time = millis();
  
  // start counting now
  TCCRnB = _BV(CS10) | _BV(CS11) | _BV(CS12);  //  Counter Clock source = pin Tn , start counting now
}

// call this to read the current count and save it
unsigned long HardwareCounter::count()
{

  TCCRnB = TCCRnB & ~7;   // Gate Off  / Counter Tn stopped
  _count = TCNTn;         // Set the count in object variable
  TCCRnB = TCCRnB | 7;    // restart counting

  return 0xfffful*g_ovf_n + _count;
}

// This indicates when the count over the determined period is over
int HardwareCounter::available()
{
  // get current time
  unsigned long now = millis();
  // do basic check for millis overflow
  if (now >= _start_time)
    return (now - _start_time >= _delay);
  else
    return (ULONG_MAX + now - _start_time >= _delay);
}

// This takes care of the counter overflow problem
ISR(TIMERn_OVF_vect)
{
  // increment number of overflows
  g_ovf_n++;
}
