#include <math.h>
#include <stdlib.h>  
#include <LiquidCrystal.h>   
#include "Counter.h"

#define TIMER_BLUE 30000    // Time (in milliseconds) of measurement with the blue light for GFP fluorescence quantification
#define TIMER_RED 500  // time (ms) for the measurement of turbidity
#define TIMER_WAIT 150 //time between each intermediary count
#define BIND_MOTOR_TO_BLUE 1
#define BIND_MOTOR_TO_RED 2

#define LED_ON LOW
#define LED_OFF HIGH

#define VERSION "Prototype v4.0"

//define the pin's used 
const int blue_led = A2;
const int red_led = A3;
const int pinCounter = 5;


/* for Vane: */
const int motor[] = {31,33,35,37,39};
const int stepPin = 40; 
/* 
 * +del+ is set to the number of milliseconds between steps (default 5 ms)
 * default: 5 
 */
const int del = 5; // ms between steps
/* 
 * +mult+ is the number of times you want to multiply your measured value to 
 * change it in milliseconds of pumping. For example, for turbidity, I know that
 * the values range from 0-1000, so I set mult = 10 to have a maximum pumping time
 * of 10 second (1000 x mult  = 10'000 ms = 1 s)
 * default: 10
 */ 
const int mult = 20; 
/* 
 * +bias+ is the number to be subtracted to your measured value (before multiplication) to 
 * change it in milliseconds of pumping. For example, for turbidity, I know that
 * the values range from 500-1000, so I set bias to 500 to have a minimum pumping time
 * of 0 second
 * default: 0
 */ 
const int bias = 0; 
/* 
 * +bind+ can be set to BIND_MOTOR_TO_BLUE or BIND_MOTOR_TO_RED
 * depending if you want to make your pumps depend on fluorescence or turbidity
 * remember to adapt +mult+ if you change this
 * default: BIND_MOTOR_TO_RED
 */
const int bind = BIND_MOTOR_TO_RED; 

int button1 = 2;
int button1_int = 0;
int iMotor = -1;
// state variables
enum State { NIL, IDLE, COUNT_BLUE, COUNT_RED, COUNT_NOISE };
State state = NIL;

// Counter functions, we have one separate counter for each state of the system.
HardwareCounter hwc(pinCounter, TIMER_BLUE);

// Associate digital pins with the LCD
// Arguments: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(9, 3, 4, 8, 6, 7);

void resetState() {
  // this function simply sets all the motor pins to HIGH
  for(int i=0;i<5;i++){
    digitalWrite(motor[i],HIGH);
  }
}

void turnMotor(int state, int timeMs){
    // this function makes motor turn during timeMs milliseconds
    // remember that motors are 0-4 and not 1-5
    resetState();
    digitalWrite(motor[state],LOW);
    int t = 1;
    while (t < timeMs){
      // loop until t is high enough
      digitalWrite(stepPin,HIGH); 
      delay(del); 
      digitalWrite(stepPin,LOW); 
      delay(del); 
      // at each iteration, increment t of the number of milliseconds we waited
      t += del*2;
    } 
}
void button_pressed()
{
  // here we start the counting process
  if (state == IDLE)
  {
    state = COUNT_BLUE;
    lcd.clear();
    lcd.print("Taking");
    lcd.setCursor(0,1);
    lcd.print("measure.");
    Serial.println("Start BLUE.");
    hwc.set_delay(TIMER_BLUE);
    digitalWrite(blue_led, LED_ON);
    hwc.start();
    // increment iMotor, but only up to 4, then goto 0 again
    iMotor = (iMotor + 1) % 5;
    
  }
}

void setup()
{
  
  Serial.begin(9600); // Prepare the serial port
  Serial.println(VERSION);
  
  pinMode(blue_led, OUTPUT);
  digitalWrite(blue_led, LED_OFF);
  pinMode(red_led, OUTPUT);
  digitalWrite(red_led, LED_OFF);

  // set all motor pins to OUTPUT mode
  for(int i=0;i<5;i++){
    pinMode(motor[i], OUTPUT);
  }
  // set all motor pins to HIGH
  resetState();
  pinMode(stepPin,OUTPUT); 

  // Prepare the button
  pinMode(button1, INPUT);
  digitalWrite(button1, HIGH);
  attachInterrupt(button1_int, button_pressed, FALLING);

  state = IDLE;

  // LCD start
  lcd.begin(8,2);
  lcd.print("BioDRW");
  lcd.setCursor(0,1);
  lcd.print("HeLLo!");
  
}

void loop()
{
  
  long count;
  int tMotor = 0;
  
  if (state != IDLE)
  {
    if (hwc.available())
    {
      switch (state)
      {
        
        case COUNT_BLUE:
          // once the fluorescence is measured, execute this code: 
          count = hwc.count();
          digitalWrite(blue_led, LED_OFF);
          Serial.print("Fluorescence count: ");
          Serial.println(count);
                   
          lcd.clear();
          lcd.print("B");
          lcd.print(count);
          
          // only if bind is set to turbidity
          if(bind == BIND_MOTOR_TO_BLUE) { 
            // set the time during which we want to pump
            tMotor = (count)*mult;
            if(tMotor < 0)
              tMotor = 0;
            Serial.print("Pumping (pump "); 
            Serial.print(iMotor);
            Serial.print(") for ");
            Serial.print(tMotor);
            Serial.println(" ms");
            // start stepping motor "iMotor" during "count" milliseconds
            turnMotor(iMotor,tMotor);
          }
          
          delay(TIMER_WAIT);
          
          // go to next state
          state = COUNT_RED;
          Serial.println("Start RED.");
          hwc.set_delay(TIMER_RED);
          digitalWrite(red_led, LED_ON);
          hwc.start();
          break;
        
        case COUNT_RED:
          // once the turbidity is measured, execute this code:
          count = hwc.count();
          digitalWrite(red_led, LED_OFF);
          Serial.print("Turbidity count: ");
          Serial.println(count);
          
          // only if bind is set to turbidity
          if(bind == BIND_MOTOR_TO_RED) { 
            // set the time during which we want to pump
            tMotor = (count)*mult;
            if(tMotor < 0)
              tMotor = 0;
            Serial.print("Pumping (pump "); 
            Serial.print(iMotor);
            Serial.print(") for ");
            Serial.print(tMotor);
            Serial.println(" ms");
            // start stepping motor "iMotor" during "count" milliseconds
            turnMotor(iMotor,tMotor);
          }
          lcd.setCursor(0,1);
          lcd.print("R");
          lcd.print(count);
          
          // go to next state
          Serial.println("Finished my job. Go to IDLE.");
          state = IDLE;
          break;
        
        case COUNT_NOISE:
        
          break;
        
        default:
          break;
      }
    }
  }
 
}
