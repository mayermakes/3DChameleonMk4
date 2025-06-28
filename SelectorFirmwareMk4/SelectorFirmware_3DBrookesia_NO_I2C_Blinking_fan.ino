// November 28, 2024

/* 3DChameleon Mk4.1 Firmware

Copyright 2024 William J. Steele

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the “Software”), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions 
of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

Single Button Press Commands (count pulses of selector)

#1 - 1
#2 - 2
#3 - 3
#4 - 4
#5 - Home and Load
#6 - Unload Last and Home
#7 - Home
#8 - Next Filament
#9 - Random Filament
 
*/

 

#include <Wire.h>
#include <SPI.h>
#include <Servo.h>
//NEW: VARIABLES:
// Define LED pins
const int ledPins[5] = {14, 4, 7, 13};

// FAN controll variables:
float tempTarget = 30.0; // Example: 30°C target temperature
const int thermistorPin = 3; // Analog pin for NTC thermistor
const int fanPin = 16;        // PWM output to control the fan

// Constants for thermistor calculation
const float seriesResistor = 10000.0; // Value of the series resistor (ohms)
const float nominalResistance = 10000.0; // Resistance at 25°C (ohms)
const float nominalTemperature = 25.0; // °C
const float betaCoefficient = 3950.0; // Beta coefficient of thermistor
const int adcMax = 1023;



// defines pins numbers -3D Brookesia // 3D Chameleon Board
// numbers after // are original pinout
#define extEnable 10 //0
#define extStep 0 // 1
#define extDir 6 //2

#define selEnable 11 //0
#define selStep 1 //1
#define selDir 5 //2

#define trigger 2 //3
#define s_limit 15 //4
#define filament 12 //5

const int counterclockwise = HIGH;
const int clockwise = !counterclockwise;

const int stepsPerRev = 200;
const int microSteps = 16;
const int speedDelay = 170;

const int defaultBackoff = 10;

Servo filamentCutter;  // create servo object to control a servo
int cutterPos = 0;    // variable to store the servo position
bool reverseServo = true;

int currentExtruder = -1;
int nextExtruder = 0;
int lastExtruder = -1;
int tempExtruder = -1;

int seenCommand = 0;
int prevCommand = 0;

int loaderMode = 2;  //(0= direct drive, 1=loader/unloader, 2=loader/unloader with press)

long triggerTime = 300;
long pulseTime = (triggerTime / 2);

long distance = 10;

long unloadDistance = stepsPerRev * microSteps * distance;  // this is 10 revs - about 10"
long loadDistance   = unloadDistance * 1.1;           // this is 11 revs - about 11"

int address = 0;
byte value;

long idleCount = 0;
bool logoActive = false;
bool T0Loaded = false;
bool T1Loaded = false;
bool T2Loaded = false;
bool T3Loaded = false;


bool displayEnabled = false;
bool ioEnabled = false;
//int sensorEnabled = 0;

long randomNumber = 0;
// read the sparkfun SX1509 io



// execute the pulse count command
void processCommand(long commandCount){

  // select case for commands
  switch (commandCount)
  {
  case 2: // unload current, switch to #0, load
    setLEDCount(1);
    currentExtruder = 0;
    processMoves();
    
    break;

  case 3: // unload current, switch to #1, load
    setLEDCount(2);
    currentExtruder = 1;
    processMoves();
    
    break;

  case 4: // unload current, switch to #3, load
    setLEDCount(3);
    currentExtruder = 2;
    processMoves();
    
    break;

  case 5: // unload current, switch to #4, load
    setLEDCount(4);
    currentExtruder = 3;
    processMoves();
    
    break;

  case 6: //home and reload #1
    
    homeSelector();
    
    gotoExtruder(0, 0);
    if(loaderMode>0)rotateExtruder(clockwise, loadDistance);
    if(loaderMode>0)gotoExtruder(0, 1);
    currentExtruder = 0;
    lastExtruder = 0;
    
    break;
    
  case 7: // unload current and rehome selector
    
    connectGillotine();
    cutFilament();
    switch(lastExtruder)
    {
      case 0:
        setLEDCount(1);
        break;
      case 1:
        setLEDCount(2);
        break;
      case 2:
        setLEDCount(3);
        break;
      case 3:
        setLEDCount(4);
        break;
    } 
    if(loaderMode>0)gotoExtruder((lastExtruder==3?2:lastExtruder+1),lastExtruder);
    if(lastExtruder<2)
    {
      if(loaderMode>0)rotateExtruder(counterclockwise, unloadDistance);
    }
    else
    {
      if(loaderMode>0)rotateExtruder(clockwise, unloadDistance);
    }
    disconnectGillotine();
    
    break;

  case 8:  
    
    homeSelector();
    
    break;

  case 9: // move to next available filament
    
    connectGillotine();
    cutFilament();
    
    currentExtruder++;
    if(currentExtruder==4)currentExtruder=0;
    processMoves();
    
    break;

  case 10: // move to a random filament
    
    connectGillotine();
    cutFilament();
    
    // select a random number
    randomNumber = random(0,2) + 1;

    // skip ahead that many tools
    for(long i=0; i<randomNumber; i++)
    {
      currentExtruder++;
      if(currentExtruder==4)currentExtruder=0;
    }
    processMoves();
    
    break;
    
  default:
    
    delay(200);

    
    break;
  }
}

// just the routine to update the OLED


// real work is here
void processMoves(){

  // make sure we have a real extruder selected
  if(lastExtruder>-1)
  {

    // if so, we need to cut the filament
    
    connectGillotine();
    cutFilament();

    // ok... then wait for the 2nd button press to unload it
    switch(lastExtruder)
    {
      case 0:
        setLEDCount(1);
        break;
      case 1:
        setLEDCount(2);
        break;
      case 2:
        setLEDCount(3);
        break;
      case 3:
        setLEDCount(4);
        break;
    } 

    // roll over to first if on last
    if( loaderMode>0 ) gotoExtruder( ( lastExtruder==3 ? 2 : (lastExtruder+1)), lastExtruder);

    // this determines which direction to move the motor, 0-1 : counterclockwise, 2-3 : clockwise
    if(lastExtruder<2)
    {
      if(loaderMode>0)rotateExtruder(counterclockwise, unloadDistance);
    }
    else
    {
      if(loaderMode>0)rotateExtruder(clockwise, unloadDistance);
    }
  }
  else
  {
    lastExtruder = 0;
  }
  disconnectGillotine();
  
  // tell it to actually execute that command now
  gotoExtruder(lastExtruder, currentExtruder);

  // ok... filament unloaded, time to load the new... so tell the user
  switch(currentExtruder)
  {
    case 0:
      setLEDCount(1);
      break;
    case 1:
      setLEDCount(2);
      break;
    case 2:
      setLEDCount(3);
      break;
    case 3:
      setLEDCount(4);
      break;
  }

  // same (but inversed) logic for motor direction
  if(currentExtruder<2)
  {
    if(loaderMode>0)rotateExtruder(clockwise, loadDistance);
  }
  else
  {
    if(loaderMode>0)rotateExtruder(counterclockwise, loadDistance);
  }

  // if we're loading, then load it now
  if(loaderMode>0)gotoExtruder(currentExtruder, (currentExtruder==3?2:currentExtruder+1));

  // everybody remember where we parked!
  lastExtruder = currentExtruder;
}


// this function simply moves from the currentCog to the targetCog is the best way
void gotoExtruder(int currentCog, int targetCog){
 
  int newCog = targetCog - currentCog;

  // ok... which way
  int newDirection = counterclockwise;
  if(newCog<0)
  {
    // we need to move the other way
    newDirection = clockwise;

    //and since we know we went too far... let's go the other way in steps as well
    newCog = currentCog - targetCog;
  }

  // if we're already on the current cog, then do nothing
  if(newCog > 0)
  {    
    // advance tool targetCog times
    for(int i=0; i<newCog; i++)
    {
      rotateSelector(newDirection, (stepsPerRev / 4) * microSteps);
    }
  }
}

// move the extruder motor in a specific direction for a specific distance (unless it's a "until button is not pressed")
void rotateExtruder(bool direction, long moveDistance){
  // note to bill:  make this acecelerate so it's very fast!!!
  
  digitalWrite(extEnable, LOW);  // lock the motor
  digitalWrite(extDir, direction); // Enables the motor to move in a particular direction
  const int fastSpeed = speedDelay/2; // double time

  // this is depricated right now... might bring it back in the future
  if(loaderMode==1)
  {

    // Makes 50 pulses for making one full cycle rotation
    for (long x = 0; x < (moveDistance-1); x++)
    {
      // this is how we pulse the motor to get it to step
      digitalWrite(extStep, HIGH);
      delayMicroseconds(fastSpeed);
      digitalWrite(extStep, LOW);
      delayMicroseconds(fastSpeed);
    }

  }

  if(loaderMode==2)
  {

   // keep waiting until button is pressed
   while (digitalRead(trigger) != 0)
    {
      delay(50);
    }
    
    // Move while button is pressed
    while (digitalRead(trigger) == 0)
    {

      // this is how we pulse the motor to get it to step
      digitalWrite(extStep, HIGH);
      delayMicroseconds(fastSpeed);
      digitalWrite(extStep, LOW);
      delayMicroseconds(fastSpeed);
    }
  }
  // ok, done pressing button, so make sure we're not energized (high is no, low is yes)
  digitalWrite(extEnable, HIGH);
}

// similar to extruder, but only stepping 50 (of 200) at a time
void rotateSelector(bool direction, int moveDistance){

  // while we are at it... can we make this faster using the magic you invented above?
  
  digitalWrite(selEnable, LOW); // lock the selector
  digitalWrite(selDir, direction); // Enables the motor to move in a particular direction

    // Makes 50 pulses for making one full cycle rotation
    for (int x = 0; x < (moveDistance-1); x++)
    {
      digitalWrite(selStep, HIGH);
      delayMicroseconds(speedDelay);
      digitalWrite(selStep, LOW);
      delayMicroseconds(speedDelay);
    }
}

// this cycles the servo between two positions
void cutFilament() {
  digitalWrite(selEnable, LOW); // disable stepper so we have power!
  if(reverseServo==false)
  {
    openGillotine();
    closeGillotine();
  }
  else
  {
    closeGillotine();
    openGillotine();
  }
  digitalWrite(selEnable, HIGH);
}

// enable the servo
void connectGillotine(){
  filamentCutter.attach(11);
}

// disable the servo - so it doesn't chatter when not in use
void disconnectGillotine(){
  filamentCutter.detach();
}

// cycle servo from 135 and 180
void openGillotine(){
    for (int pos = 135; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    filamentCutter.write(pos);              // tell servo to go to position in variable 'pos'
    delayMicroseconds(25000);                       // waits 15ms for the servo to reach the position
  }
  //filamentCutter.write(3.5);       // tell servo to go to position in variable 'pos'
  delay(50);                       // waits 15ms for the servo to reach the position
}

// reverse cycle servo from 180 back to 135
void closeGillotine(){
  for (int pos = 180; pos >= 135; pos -= 1) { // goes from 180 degrees to 0 degrees
    filamentCutter.write(pos);              // tell servo to go to position in variable 'pos'
    delayMicroseconds(25000);                       // waits 15ms for the servo to reach the position
  }
  delay(50);                       // waits 15ms for the servo to reach the position
}

// rotate the selector clockwise too far from 4, so it'll grind on the bump stop
void homeSelector(){
  // rotate counter clockwise to hard stop
  rotateSelector(clockwise, stepsPerRev * microSteps);
  setLEDCount(4);
  delay(250);
  setLEDCount(0);
  delay(250);
  setLEDCount(4);
  delay(250);
  setLEDCount(0);
  

  // move just slightly to extruder 1 (this backs off a little from the hard stop)
  rotateSelector(counterclockwise, defaultBackoff * microSteps);

 currentExtruder = 0;
 lastExtruder = -2;

}

// buzz buzz buzz
void vibrateMotor(){
  // oscillate selector 1 time
  rotateSelector(clockwise, 2 * 16);
  rotateSelector(!clockwise, 2 * 16);
}
//NEW functions:

void controlFan() {
  int adcValue = analogRead(thermistorPin);
  
  // Convert ADC value to resistance
  float resistance = seriesResistor / ((adcMax / (float)adcValue) - 1);

  // Calculate temperature using Beta parameter equation
  float temperatureK = 1.0 / (1.0 / (nominalTemperature + 273.15) + 
                      (1.0 / betaCoefficient) * log(resistance / nominalResistance));
  float temperatureC = temperatureK - 273.15;

  // Control fan speed (simple proportional control)
  int pwmValue = 0;
  if (temperatureC > tempTarget) {
    float diff = temperatureC - tempTarget;
    pwmValue = 255; // Scale as needed
  }

  analogWrite(fanPin, pwmValue);
}

// Function to light up 0 to 4 LEDs
void setLEDCount(int count){
  // Clamp the count to valid range (0–4)
  count = constrain(count, 0, 4);

  // First, turn all LEDs off
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  for (int i = 0; i < count; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
}


void setup(){

  seenCommand = 0;

  // Sets the two pins as Outputs
  pinMode(extEnable, OUTPUT);
  pinMode(extStep, OUTPUT);
  pinMode(extDir, OUTPUT);

  pinMode(selEnable, OUTPUT);
  pinMode(selStep, OUTPUT);
  pinMode(selDir, OUTPUT);
  
  // set up the button
  pinMode(trigger, INPUT_PULLUP);  // selector

  // a little override here... we're using the two inputs as I2C instead
  pinMode(s_limit, OUTPUT);    
  pinMode(filament, OUTPUT); 
  //NEW PINS:
  pinMode(ledPins[0], OUTPUT);
  pinMode(ledPins[1], OUTPUT);
  pinMode(ledPins[2], OUTPUT);
  pinMode(ledPins[3], OUTPUT);
  setLEDCount(1);
  delay(500);
  setLEDCount(2);
  delay(500);
  setLEDCount(3);
  delay(500);
  setLEDCount(4);
  delay(500);
  setLEDCount(0);
  delay(100);
  // lock the selector by energizing it
  digitalWrite(selEnable, HIGH);

  // make sure filament isn't blocked by gillotine
  connectGillotine();
  cutFilament();
  disconnectGillotine();
  
  prevCommand = 0;


}



void loop(){
  static uint32_t lastTime = 0;

  seenCommand = 0;
  idleCount++;

  // process button press
  if (digitalRead(trigger) == 0)
  {
    idleCount = 0;
    logoActive = false;
    unsigned long nextPulse;
    unsigned long pulseCount = 0;
    unsigned long commandCount = 0;

    // keep counting (and pulsing) until button is released
    while (digitalRead(trigger) == 0)
    {
      if(pulseCount<pulseTime)
      {
        pulseCount++;
        
        if(pulseCount>1) vibrateMotor();
      }
      delay(400);  // each pulse is 400+ milliseconds apart 
    }
    processCommand(pulseCount); // ok... execute whatever command was caught (by pulse count)
    pulseCount = 0;
  }

  //digitalWrite(fanPin,LOW);
  controlFan();
  // each loop adds 50ms delay, so that gets added AFTER the command is processed before the next one can start
  delay(50);
}
