/* 
 * Project: Kiln Controller 
 * Author: Rebecca Snyder
 * Date: 2023-11-30
 * 
 * 
 * use ctrl-shift-p to install "MAX6675_CNM", particle library for thermocouple board 
 * 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "MAX6675.h"
float tempC, tempF;


MAX6675 thermocouple;
byte thermocoupleStatus;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);




void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);
  thermocouple.begin(SCK, SS, MISO);


  thermocoupleStatus = thermocouple.read();
  if (thermocoupleStatus != STATUS_OK) {
    Serial.printf("ERROR WITH THERMOCOUPLE!\n");
  }
  else {
    Serial.printf("thermocouple %d",thermocoupleStatus);
  }
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  
}
 
