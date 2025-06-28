// 19.6.25 Remix to fit new electronic variant 
//3D brookesia (thats a tiny chameleon...because it runs on Attiny)

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

//Don´t mind if I do. thanks William.

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



// Define LED pins
const int ledPins[4] = {14, 4, 7, 13};

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





//Stepper:
int totalStepsPerRotation = 200*16;
int stepDelay = 50;

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
    pwmValue = min(255, (int)(diff * 25)); // Scale as needed
  }

  analogWrite(fanPin, pwmValue);
}

// Function to light up 0 to 4 LEDs
void setLEDCount(int count) {
  // Clamp the count to valid range (0–4)
  count = constrain(count, 0, 4);

  // First, turn all LEDs off
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  // Then, light up the required number of LEDs
  for (int i = 0; i < count; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
}
void homeSelector(){
  
}
void moveSelector(int target){
  //target position 1-4
}

void moveExtruder(int rotations,bool directions){
  long totalSteps = totalStepsPerRotation * rotations;
digitalWrite(extEnable, LOW);
digitalWrite(extDir, directions);
  for (long i = 0; i < totalSteps; i++) {
    digitalWrite(extStep, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(extStep, LOW);
    delayMicroseconds(stepDelay);
  }
}


void setup()
{
  pinMode(fanPin, OUTPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
    delay(250);
    digitalWrite(ledPins[i], LOW);
  } 
  
  

  

  
  // Sets the two pins as Outputs
  pinMode(extEnable, OUTPUT);
  pinMode(extStep, OUTPUT);
  pinMode(extDir, OUTPUT);

  pinMode(selEnable, OUTPUT);
  pinMode(selStep, OUTPUT);
  pinMode(selDir, OUTPUT);

  // set up the button
  pinMode(trigger, INPUT);  // selector

  
  pinMode(s_limit, INPUT_PULLUP);    
  pinMode(filament, INPUT_PULLUP); 

  // lock the selector by energizing it
  digitalWrite(selEnable, HIGH);

  


}

void loop(){
  //controlFan();
  
  bool triggered = digitalRead(trigger);
  setLEDCount(triggered);
  moveExtruder(1,trigger);
}
