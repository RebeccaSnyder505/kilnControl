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
#include "math.h"

//MAX 6675 K-thermocouple converter
const int SCKPIN = D17; // serial clock
const int CSPIN = D18; // SS or CS select
const int SOPIN = D16; // MISO or CIPO 
const int buttonPin = D6;

//MAX6675 thermocouple(SCKPIN, CSPIN, SOPIN);  // MAX6675(int8_t SCLK, int8_t CS, int8_t MISO);
byte spiread(void);
float readFahrenheit(void);
float readCelsius(void);



int readTemperatureThermocouple (*thermoArray);



// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);



void setup() {
  SPI.begin(CSPIN);
  Serial.begin(9600);
  delay(500);
  pinMode(SCKPIN, OUTPUT);
  pinMode(CSPIN, OUTPUT);
  pinMode(SOPIN, OUTPUT);
  pinMode(buttonPin, OUTPUT);


}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  Serial.printf("c = %f \n", readCelsius());
  Serial.printf("f = %f \n", readFahrenheit());
  delay(5000); // must delay minimum of 250ms
}

int readTemperatureThermocouple(*thermoArray) {
  digitalWrite(CSPIN,LOW);
  int i = 0;
  for (i=0;i<=11;i++) {
    thermoArray[i] = SPI1.transfer();
    Serial.printf("place %d is value %d \n",i,thermoArray[i]);
  }
}

float readCelsius(void) {
  uint16_t v;
  digitalWrite(CSPIN, LOW);
  delayMicroseconds(10);
  v = spiread();
  v <<= 8;
  v |= spiread();
  digitalWrite(CSPIN, HIGH);
  if (v & 0x4) {
    // uh oh, no thermocouple attached!
    return NAN;
    // return -100;
  }
  v >>= 3;
  return v * 0.25;
}


float readFahrenheit(void) { return readCelsius() * 9.0 / 5.0 + 32; }



byte spiread(void) {
  int i;
  byte d = 0;
  for (i = 7; i >= 0; i--) {
    digitalWrite(CSPIN, LOW); // SCKPIN? perhaps is wrong
    delayMicroseconds(10);
    if (digitalRead(SOPIN)) {
      // set the bit to 0 no matter what
      d |= (1 << i);
    }
    digitalWrite(CSPIN, HIGH);
    delayMicroseconds(10);
  }
  return d;
}





