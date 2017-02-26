/*******************************************************************************
LED binary clock on Arduino Pro Mini and DS1302 Real Time Clock chip
http://libahunt.ee/binaryclock
by Anna Jõgi a.k.a Libahunt

Using some code from http://playground.arduino.cc/Main/DS1302
By arduino.cc user "Krodal".


PIN CONNECTIONS   

LED matrix looks like this:
  O   O   O
  O O O O O
O O O O O O
O O O O O O

LED columns:
I (hours ten-part)      A3
II (hours one-part)     A2
III (minutes ten-part)  A1
IV (minutes one-part)   A0
V (seconds ten-part)    13
VI (seconds one-part)   12

LED rows:
1. (8)                    9
2. (4)                    8
3. (2)                    10
4. (1)                    11

Increment button          7
Set button                6
(buttons have pulldown resistors)

DS1302 RTC:
SCLK (pin 7 on DS1302)    2
I/O (pin 6 on DS1302)     3
CE (pin 5 on DS1302)      4

*/

#include "BCDtime.h"
#include "DS1302.h"
#include "DebouncedButtons.h"

#define DS1302_SCLK_PIN   2   
#define DS1302_IO_PIN     3
#define DS1302_CE_PIN     4

#define BUTTON_INCREMENT_PIN   7
#define BUTTON_SET_PIN         6


#define TICKING     0
#define SET_HOUR    1
#define SET_MINUTE  2

int mode = TICKING;

const int frequency = 2500;//how many timer interrupts to count for an approximate 1 second
int colCounter = 0;
int colValue = 0;
int interruptCounter = 0;
int updateCounter = 0;
int updateIntervalSec = 3;//5*60; //Controller counts time this long on it's own and then updates from RTC


volatile boolean interrupt = false; //helper to do actual computing in loop() instead of interrupt routine

ds1302_struct rtc;

BCDTime t;

DebouncedButton buttonIncrement = DebouncedButton(BUTTON_INCREMENT_PIN,50,0);
DebouncedButton buttonSet = DebouncedButton(BUTTON_SET_PIN,50,0);
//50 is debounce delay, 0 indicates pull down (as opposed to pull up resistor)



void setup() {
  
  /*pin modes*/
  DDRC |= 0x0F; //Outputs A3, A2, A1, A0;
  DDRB |= 0x1F; //Outputs 13, 12, 11, 10, 9, 8;
  
  DDRD &= ~0xC0;//Inputs 7, 6; DebouncedButtons class does not take care of pinmode.
  PORTD &= ~0xC0;//Turn their internal pull up resistors off, because we use external pull downs.

  PORTD &= ~0x07; //Also turn off internal pullups for the DS1302 module.
  
  PORTC &= ~0x0F; //Switch outputs off.
  PORTB &= ~0x3F;
  
  
  /*TIMER INTERRUPT
  from http://www.instructables.com/id/Arduino-Timer-Interrupts/ */
  cli();//stop interrupts
  
  //set timer2 interrupt at 2.5kHz
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 2.5khz increments
  //compare match register = [ clock_speed_in_hz/ (prescaler * desired interrupt frequency) ] - 1
  OCR2A = 99;// = (16*10^6) / (64 * 2.5*10^3) - 1 (must be <256)
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (0 << CS22) | (1 << CS21) | (1 << CS20);// prescaler 64
  TIMSK2 |= (1 << OCIE2A);                           
  sei();//allow interrupts

  t.setTime(0,0,0);
  getTimeFromDS1302();//Should put a new valu into t, if already ticking.
  if (t.h==0 && t.m == 0 && t.s == 0) {
    setTimeInDS1302(); //Setting time makes the RTC start to tick.
  }

  
} //end of setup


/*
Time counting an lighting up leds takes place 
in loop when "interrupt" variable is set
*/
ISR(TIMER2_COMPA_vect){ //timer2 interrupt 2.5kHz
  
  interrupt = true;//good practice to keep code in interrupt routine as small as possible
                   //each time this variable is set, loop() will handle things
  
} //end of ISR




void loop() {

  if (mode == TICKING) {
    if(buttonSet.dbReadPushStarted()) {
      t.setSeconds(70);
      mode = SET_HOUR;
    }
  }

  if (mode == SET_HOUR) {
    if(buttonSet.dbReadPushStarted()) {
      t.setSeconds(7);
      mode = SET_MINUTE;
    }
    else if(buttonIncrement.dbReadPushStarted()) {
      t.incrementHour();
    }
  }

  if (mode == SET_MINUTE) {      
    if(buttonSet.dbReadPushStarted()) {
      t.setSeconds(0);
      noInterrupts();
      setTimeInDS1302();//writes t variable contents into RTC chip
      mode = TICKING;
      interrupts();
    }
    else if(buttonIncrement.dbReadPushStarted()) {
      t.incrementMin();
    }
  }

  if (interrupt) {

    //On an approximate 1 second, increase clock variable.
    //interruptCounter is not incremented while in setting routine
    if (interruptCounter >= frequency) {
      t.tick();
      interruptCounter = 0;
      updateCounter++;
    }

    if (updateCounter >= updateIntervalSec) {
      DPL("Time to update.");
      getTimeFromDS1302();
      updateCounter = 0;
    }

    //Output current column.
    colValue = t.getBCDcomponent(colCounter);

    if (colValue/8 > 0) {
      PORTB |= 0x02;
      colValue -= 8;
    }
    else PORTB &= ~0x02;
    
    if (colValue/4 > 0) {
      PORTB |= 0x01;
      colValue -= 4;
    }
    else PORTB &= ~0x01;
    
    if (colValue/2 > 0) {
      PORTB |= 0x04;
      colValue -= 2;
    }
    else PORTB &= ~0x04;
    
    if (colValue > 0) PORTB |= 0x08;
    else PORTB &= ~0x08;
    
    
    switch (colCounter) {
      case 0:
        PORTC |= 0x08;
        PORTC &= ~0x07;
        PORTB &= ~0x30;
        break;
      case 1:
        PORTC |= 0x04;
        PORTC &= ~0x0B;
        PORTB &= ~0x30;
        break;
      case 2:
        PORTC |= 0x02;
        PORTC &= ~0x0D;
        PORTB &= ~0x30;
        break;
      case 3:
        PORTC |= 0x01;
        PORTC &= ~0x0E;
        PORTB &= ~0x30;
        break;
      case 4:
        PORTB |= 0x20;
        PORTC &= ~0x0F;
        PORTB &= ~0x10;
        break;
      case 5:
        PORTB |= 0x10;
        PORTC &= ~0x0F;
        PORTB &= ~0x20;
    }
    
    
    //Move colCounter for next round
    colCounter++;
    if (colCounter > 5) {
      colCounter = 0;
    }

    //interruptCounter is not incremented while in setting routine
    if (mode == TICKING) {
      interruptCounter++;
    }
    
    interrupt = false;
  }

}// /loop()


void setTimeInDS1302() {

  // Start by clearing the Write Protect bit
  DS1302_write (DS1302_ENABLE, 0);
  // Disable Trickle Charger.
  DS1302_write (DS1302_TRICKLE, 0x00);

  memset ((char *) &rtc, 0, sizeof(rtc));

  rtc.Seconds    = t.getBCDcomponent(5);
  rtc.Seconds10  = t.getBCDcomponent(4);
  rtc.CH         = 0;      // 1 for Clock Halt, 0 to run;
  rtc.Minutes    = t.getBCDcomponent(3);
  rtc.Minutes10  = t.getBCDcomponent(2);
  rtc.h24.Hour   = t.getBCDcomponent(1);
  rtc.h24.Hour10 = t.getBCDcomponent(0);
  rtc.h24.hour_12_24 = 0; // 0 for 24 hour format
  //Date does not concern us, write something random there
  rtc.Date       = 1;
  rtc.Date10     = 0;
  rtc.Month      = 1;
  rtc.Month10    = 0;
  rtc.Day        = 0;
  rtc.Year       = 0;
  rtc.Year10     = 0;
  rtc.WP = 0;  

  // Write all clock data at once (burst mode).
  DS1302_clock_burst_write( (uint8_t *) &rtc);
  
}

void getTimeFromDS1302() {
  // Read all clock data at once (burst mode).
  DS1302_clock_burst_read( (uint8_t *) &rtc);
  DP("RTC read: ");
  DP(rtc.h24.Hour10);
  DP(rtc.h24.Hour);
  DP(rtc.Minutes10);
  DP(rtc.Minutes);
  DP(rtc.Seconds10);
  DPL(rtc.Seconds);
  t.setBCD(rtc.h24.Hour10, rtc.h24.Hour, rtc.Minutes10, rtc.Minutes,rtc.Seconds10, rtc.Seconds);
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
