/* 
 * Project: Kiln Controller via MAX6675 K-Thermocouple Converter (0°C to +1024°C)
 * Author: Rebecca Snyder
 * Date: 2023-11-27
 * 
 * 
 * 
 * must install library "Adafruit_SSD1306" with ctrl-shift-p
 * must install libary "IoTClassroom_CNM" with ctrl-shift-p
 */


#include "Particle.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Button.h"

//MAX 6675 K-thermocouple converter
const int SCKPIN = D17;
const int CSPIN = D15;
const int SOPIN = D16;
const int buttonPin = D6;

int thermoArray[12];

//int readTemperatureThermocouple (*thermoArray);



// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);



void setup() {
  Serial.begin(9600);
  pinMode(SCKPIN, INPUT);
  pinMode(CSPIN, INPUT);
  pinMode(SOPIN, INPUT);
  pinMode(buttonPin, INPUT);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
//  readTemperatureThermocouple(*thermoArray);
  int i = 0;
  for (i=0;i<=11;i++) {
    Serial.printf("%d %d %d \n", digitalRead(SCKPIN), digitalRead(CSPIN), digitalRead(SOPIN));

//   thermoArray[i] = digitalRead(SOPIN);
//    Serial.printf("place %d is value %d \n",i,thermoArray[i]);
    delay(100);
  }
}

// int readTemperatureThermocouple(*thermoArray) {
//   int i = 0;
//   for (i=0;i<=11;i++) {
//     thermoArray[i] = digitalread(SOPIN);
//     Serial.printf("place %d is value %d \n",i,thermoArray[i]);
//   }
// }





