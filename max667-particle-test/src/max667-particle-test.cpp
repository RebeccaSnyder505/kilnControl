/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "SPI.h"

#include "max6675-particle.h"

int thermoDO = D16; // CHANGE TO PINS
int thermoCS = D18;
int thermoCLK = D17;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

  
void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,5000);
  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  // basic readout test, just print the current temp
  
   Serial.print("C = "); 
   Serial.println(thermocouple.readCelsius());
   Serial.print("F = ");
   Serial.println(thermocouple.readFahrenheit());
 
   delay(1000);
}